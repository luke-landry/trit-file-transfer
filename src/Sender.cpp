
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
#include "WorkerContext.h"
#include "staging.h"

Sender::Sender(const std::string& receiver_ip, const uint16_t receiver_port, const crypto::Key& key,const crypto::Salt& salt): 
    receiver_ip_(receiver_ip),
    receiver_port_(receiver_port),
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
    receiver_socket_.connect(receiver_ip_, receiver_port_);
    std::cout << "Connected to " << receiver_ip_ << ":" << receiver_port_ << std::endl;

}

// Handshake verifies matching keys were derived between sender and receiver (from matching passwords)

// Handshake format: salt, nonce, cipher, header
bool Sender::send_handshake(const crypto::Encryptor& encryptor){
    LOG("sending handshake");
    
    receiver_socket_.write(salt_.data(), salt_.size());
    LOG("sent salt");

    LOG("creating handshake nonce and cipher");
    auto [nonce, cipher] = crypto::encrypt_handshake_tag(key_);

    // LOG("nonce=" + utils::buffer_to_hex_string(nonce.data(), nonce.size()));
    // LOG("cipher=" + utils::buffer_to_hex_string(cipher.data(), cipher.size()));

    receiver_socket_.write(nonce.data(), nonce.size());
    LOG("handshake nonce sent");

    receiver_socket_.write(cipher.data(), cipher.size());
    LOG("handshake cipher sent");

    LOG("sending encryption header");
    const auto& header = encryptor.header();
    receiver_socket_.write(header.data(), header.size());
    // LOG("header=" + utils::buffer_to_hex_string(header.data(), header.size()));

    std::cout << "Handshake sent" << std::endl;

    // Get success/fail response
    uint8_t handshake_success_byte;
    receiver_socket_.read(&handshake_success_byte, sizeof(handshake_success_byte));
    return static_cast<bool>(handshake_success_byte);
}

TransferRequest Sender::create_transfer_request(){
    return TransferRequest::from_file_paths(staging::get_staged_files());
}

bool Sender::send_transfer_request(const TransferRequest& transfer_request){
    // Serialize and then send transfer request to receiver
    std::vector<uint8_t> transfer_request_buffer = transfer_request.serialize();
    uint64_t transfer_request_buffer_size = transfer_request_buffer.size();
    receiver_socket_.write(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size));
    receiver_socket_.write(transfer_request_buffer.data(), transfer_request_buffer.size());
    std::cout << "Transfer request sent" << std::endl;

    // Get accept/deny response
    uint8_t request_accepted_byte;
    receiver_socket_.read(&request_accepted_byte, sizeof(request_accepted_byte));
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

    WorkerContext ctx;
    
    FileManager file_chunker;
    std::thread chunker_thread([&](){
        try{
            file_chunker.read_files_into_chunks(
                ctx,
                transfer_request,
                file_chunk_queue,
                file_chunking_done
            );
        } catch (...){ ctx.handle_exception(); }
    });

    EncryptionManager chunk_encryptor(
        transfer_request.get_chunk_size(),
        transfer_request.get_final_chunk_size(),
        num_chunks,
        std::move(encryptor)
    );
    std::thread encryption_thread([&](){
        try{
            chunk_encryptor.encrypt_chunks(
                ctx,
                file_chunk_queue,
                file_chunking_done,
                encrypted_chunk_queue,
                encryption_done
            );
        } catch (...){ ctx.handle_exception(); }
    });

    TransferManager chunk_sender;
    std::thread transmission_thread([&](){
        try{
            chunk_sender.send_chunks(
                ctx,
                receiver_socket_,
                encrypted_chunk_queue,
                encryption_done,
                chunks_sent
            );
        } catch (...){ ctx.handle_exception(); }
    });

    ProgressTracker<uint32_t> progress_tracker("Chunks sent", num_chunks);
    std::thread progress_thread([&](){
        try{
            progress_tracker.start(ctx, chunks_sent);
        } catch (...){ ctx.handle_exception(); }
    });

    chunker_thread.join();
    encryption_thread.join();
    transmission_thread.join();
    progress_thread.join();

    // Handling if the transfer stopped due to an exception in any of the workers
    try{
        ctx.rethrow_if_exception();
    } catch (const std::exception& e) {
        std::cerr << "Transfer failed: " << e.what() << '\n';
        return;
    }

    auto end_time = std::chrono::system_clock::now();
    auto time_elapsed = std::chrono::duration<double>(end_time-start_time);
    auto seconds_elapsed = time_elapsed.count();

    std::cout << "Files sent, transfer complete!" << std::endl;
    std::cout << "Time elapsed: " << seconds_elapsed << "s" << std::endl;
}
