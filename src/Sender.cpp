
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#include "Sender.h"
#include "utils.h"
#include "FileManager.h"
#include "TransferManager.h"
#include "ProgressTracker.h"
#include "CompressionManager.h"
#include "EncryptionManager.h"
#include "staging.h"

Sender::Sender(const std::string& ip_address_str, const uint16_t port, const crypto::Key& key,const crypto::Salt& salt): 
    io_context_(),
    receiver_endpoint_(asio::ip::address::from_string(ip_address_str), port),
    socket_(io_context_),
    key_(key),
    salt_(salt) {}

void Sender::start_session(){
    LOG("sender session started");
    
    if(staging::get_staged_files().size() == 0){
        std::cout << "No files staged. Nothing to send." << std::endl;
        return;
    }

    connect_to_receiver();
    LOG("connected to receiver");
    
    crypto::Encryptor encryptor(key_);
    if(!send_handshake(encryptor)){
        std::cout << "Handshake failed. Ensure passwords match." << std::endl;
        return;
    }
    LOG("handshake successful");

    TransferRequest transfer_request = create_transfer_request();
    LOG("transfer request created");

    LOG("sending transfer request");
    if(!send_transfer_request(transfer_request)){
        std::cout << "Transfer was declined by the receiver" << std::endl;
        LOG("transfer request denied by receiver");
        return;
    }
    LOG("transfer request accepted by receiver");

    LOG("starting transfer send");
    send_files(transfer_request, std::move(encryptor));
    LOG("transfer sent and completed");
}

void Sender::connect_to_receiver(){
    const std::string receiver_address_str = receiver_endpoint_.address().to_string();
    uint16_t receiver_port = receiver_endpoint_.port();
    socket_.connect(receiver_endpoint_);
    std::cout << "Connected to " << receiver_address_str << ":" << receiver_port << std::endl;
}

// Handshake verifies matching keys were derived between sender and receiver (from matching passwords)

// Handshake format: salt, nonce, cipher, header
bool Sender::send_handshake(const crypto::Encryptor& encryptor){
    LOG("sending handshake");
    
    asio::write(socket_, asio::buffer(salt_.data(), salt_.size()));
    LOG("sent salt");

    LOG("creating handshake nonce and cipher");
    auto [nonce, cipher] = crypto::encrypt_handshake_tag(key_);

    // LOG("nonce=" + utils::buffer_to_hex_string(nonce.data(), nonce.size()));
    // LOG("cipher=" + utils::buffer_to_hex_string(cipher.data(), cipher.size()));

    asio::write(socket_, asio::buffer(nonce.data(), nonce.size()));
    LOG("handshake nonce sent");

    asio::write(socket_, asio::buffer(cipher.data(), cipher.size()));
    LOG("handshake cipher sent");

    LOG("sending encryption header");
    const auto& header = encryptor.header();
    asio::write(socket_, asio::buffer(header.data(), header.size()));
    // LOG("header=" + utils::buffer_to_hex_string(header.data(), header.size()));

    std::cout << "Handshake sent" << std::endl;

    // Get success/fail response
    uint8_t handshake_success_byte;
    asio::read(socket_, asio::buffer(&handshake_success_byte, sizeof(handshake_success_byte)));
    return static_cast<bool>(handshake_success_byte);
}

TransferRequest Sender::create_transfer_request(){
    return TransferRequest::from_file_paths(staging::get_staged_files());
}

bool Sender::send_transfer_request(const TransferRequest& transfer_request){
    // Serialize and then send transfer request to receiver
    std::vector<uint8_t> transfer_request_buffer = transfer_request.serialize();
    uint64_t transfer_request_buffer_size = transfer_request_buffer.size();
    asio::write(socket_, asio::buffer(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size)));
    asio::write(socket_, asio::buffer(transfer_request_buffer));
    std::cout << "Transfer request sent" << std::endl;

    // Get accept/deny response
    uint8_t request_accepted_byte;
    asio::read(socket_, asio::buffer(&request_accepted_byte, sizeof(request_accepted_byte)));
    return static_cast<bool>(request_accepted_byte);
}

void Sender::send_files(const TransferRequest& transfer_request, crypto::Encryptor encryptor){
    std::cout << "Sending files..." << std::endl;
    auto start_time = std::chrono::system_clock::now();

    uint32_t num_chunks = transfer_request.get_num_chunks();

    constexpr int QUEUE_CAPACITY = 50;
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> file_chunk_queue(QUEUE_CAPACITY);
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> encrypted_chunk_queue(QUEUE_CAPACITY);

    // Completion flags and progress
    std::atomic<bool> file_chunking_done(false);
    std::atomic<bool> encryption_done(false);
    std::atomic<uint32_t> chunks_sent(0);
    
    FileManager file_chunker;
    std::thread chunker_thread([&](){
        file_chunker.read_files_into_chunks(
            transfer_request,
            file_chunk_queue,
            file_chunking_done
        );
    });

    EncryptionManager chunk_encryptor(
        transfer_request.get_chunk_size(),
        transfer_request.get_final_chunk_size(),
        num_chunks,
        std::move(encryptor)
    );
    std::thread encryption_thread([&](){
        chunk_encryptor.encrypt_chunks(
            file_chunk_queue,
            file_chunking_done,
            encrypted_chunk_queue,
            encryption_done
        );
    });

    TransferManager chunk_sender;
    std::thread transmission_thread([&](){
        chunk_sender.send_chunks(
            socket_,
            encrypted_chunk_queue,
            encryption_done,
            chunks_sent
        );
    });

    ProgressTracker<uint32_t> progress_tracker("Chunks sent", num_chunks);
    std::thread progress_thread([&](){
        progress_tracker.start(chunks_sent);
    });

    chunker_thread.join();
    encryption_thread.join();
    transmission_thread.join();
    progress_thread.join();

    auto end_time = std::chrono::system_clock::now();
    auto time_elapsed = std::chrono::duration<double>(end_time-start_time);
    auto seconds_elapsed = time_elapsed.count();

    std::cout << "Files sent, transfer complete!" << std::endl;
    std::cout << "Time elapsed: " << seconds_elapsed << "s" << std::endl;
}
