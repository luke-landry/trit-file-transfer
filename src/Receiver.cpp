#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

#include "Receiver.h"
#include "utils.h"
#include "TransferManager.h"
#include "FileManager.h"
#include "ProgressTracker.h"
#include "CompressionManager.h"
#include "EncryptionManager.h"

#ifdef __linux__
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

Receiver::Receiver(const std::string& ip, uint16_t port, const std::string& password):
    ip_(ip),
    port_(port),
    password_(password) {}

void Receiver::start_session(){
    LOG("receiver session started");

    while(true){
        std::cout << "Waiting for connections..." << std::endl;
        wait_for_connection();
        LOG("connected to sender");

        std::optional<crypto::Decryptor> decryptor_opt;
        if(!receive_handshake(decryptor_opt)){
            std::cout << "Handshake failed. Ensure passwords match." << std::endl;
            continue;
        }
        LOG("handshake successful");

        TransferRequest transfer_request = receive_transfer_request();
        LOG("receieved transfer request");
        if(!accept_transfer_request(transfer_request)){
            LOG("transfer request denied by user");
            continue;
        }
        LOG("transfer request accepted by user");

        LOG("starting transfer receive");
        receive_files(transfer_request, std::move(*decryptor_opt));
        LOG("transfer receieved and completed");

        break;
    }
}

// Waits for a successful connection to be established
void Receiver::wait_for_connection(){
    if (sender_socket_.is_open()) { sender_socket_.close(); }
    std::cout << "Listening for connection at " << ip_ << " on port " << port_ << "..." << std::endl;
    TcpSocket::accept(port_, sender_socket_);
    std::cout << "Connected to " << sender_socket_.remote_endpoint_address() << " on port " << sender_socket_.remote_endpoint_port() << std::endl;
}

// Handshake format: salt, nonce, cipher, header
bool Receiver::receive_handshake(std::optional<crypto::Decryptor>& decryptor_opt){
    LOG("receiving handshake");

    LOG("receiving salt");
    std::array<uint8_t, crypto::SALT_SIZE> salt_buffer;
    sender_socket_.read(salt_buffer.data(), salt_buffer.size());
    crypto::Salt salt(salt_buffer);

    // LOG("salt=" + utils::buffer_to_hex_string(salt.data(), salt.size()));

    LOG("deriving key");
    crypto::Key key(password_, salt);

    // LOG("key=" + utils::buffer_to_hex_string(key.data(), key.size()));

    LOG("receiving handshake nonce");
    std::array<uint8_t, crypto::NONCE_SIZE> nonce_buffer;
    sender_socket_.read(nonce_buffer.data(), nonce_buffer.size());
    crypto::Nonce nonce(nonce_buffer);
    // LOG("nonce=" + utils::buffer_to_hex_string(nonce.data(), nonce.size()));

    LOG("receiving handshake cipher");
    std::array<uint8_t, crypto::HANDSHAKE_CIPHERTEXT_SIZE> ciphertext;
    sender_socket_.read(ciphertext.data(), ciphertext.size());
    // LOG("ciphertext=" + utils::buffer_to_hex_string(ciphertext.data(), ciphertext.size()));

    LOG("receiving encryption header");
    std::array<uint8_t, crypto::HEADER_SIZE> header;
    sender_socket_.read(header.data(), header.size());
    // LOG("header=" + utils::buffer_to_hex_string(header.data(), header.size()));

    bool handshake_success = crypto::verify_handshake_tag(key, nonce, ciphertext);
    if (handshake_success) { decryptor_opt.emplace(key, header); }
    uint8_t response_byte = handshake_success ? 1 : 0;
    sender_socket_.write(&response_byte, sizeof(response_byte));
    return handshake_success;
}

/*
    This function waits for, receives, and deserializes the file transfer request from the sender
    --------------------------
    Transfer request format:
    transfer reqeust size [8 bytes]

    number of files [4 bytes]
    total transfer size [8 bytes]
    uncompressed chunk size [4 bytes]
    uncompressed last chunk size [4 bytes]
    num chunks [4 bytes]
    file1 path length [2 bytes]
    file1 path [variable]
    file1 size [8 bytes]
    file2 path length [2 bytes]
    file2 path [variable]
    file2 size [8 bytes]
    ...
    fileN path length [2 bytes]
    fileN path [variable]
    fileN size [8 bytes]
*/
TransferRequest Receiver::receive_transfer_request(){
    std::cout << "Awaiting file transfer request..." << std::endl;
    uint64_t transfer_request_buffer_size;
    sender_socket_.read(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size));
    std::vector<uint8_t> transfer_request_buffer(transfer_request_buffer_size);
    sender_socket_.read(transfer_request_buffer.data(), transfer_request_buffer.size());
    return TransferRequest::deserialize(transfer_request_buffer);
}

bool Receiver::accept_transfer_request(const TransferRequest& transfer_request){
    transfer_request.print();
    char choice = utils::input<char>("Accept transfer request? (y/n)", {'y', 'n'});
    bool request_accepted = choice == 'y';
    uint8_t request_accepted_byte = request_accepted;
    sender_socket_.write(&request_accepted_byte, sizeof(request_accepted_byte));
    std::cout << ((request_accepted ? "Transfer accepted" : "Transfer denied")) << std::endl;
    return request_accepted;
}

void Receiver::receive_files(const TransferRequest& transfer_request, crypto::Decryptor decryptor){
    std::cout << "Receiving files..." << std::endl;
    auto start_time = std::chrono::system_clock::now();

    uint32_t num_chunks = transfer_request.get_num_chunks();

    constexpr int QUEUE_CAPACITY = 50;
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> received_chunk_queue(QUEUE_CAPACITY);
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> decrypted_chunk_queue(QUEUE_CAPACITY);

    std::atomic<bool> chunk_reception_done(false);
    std::atomic<bool> decryption_done(false);
    std::atomic<uint32_t> chunks_written(0);

    WorkerContext ctx;

    TransferManager chunk_receiver;
    std::thread receiver_thread([&](){
        try{
            chunk_receiver.receive_chunks(
                ctx,
                sender_socket_,
                received_chunk_queue,
                chunk_reception_done,
                num_chunks
            );
        } catch (...){ ctx.handle_exception(); }
        
    });

    EncryptionManager chunk_decryptor(
        transfer_request.get_chunk_size(),
        transfer_request.get_final_chunk_size(),
        num_chunks,
        std::move(decryptor)
    );
    std::thread decryption_thread([&](){
        try {
            chunk_decryptor.decrypt_chunks(
                ctx,
                received_chunk_queue,
                chunk_reception_done,
                decrypted_chunk_queue,
                decryption_done
            );
        } catch (...){ ctx.handle_exception(); }
    });

    FileManager file_writer;
    std::thread writer_thread([&](){
        try{
            file_writer.write_files_from_chunks(
                ctx,
                transfer_request,
                decrypted_chunk_queue,
                decryption_done,
                chunks_written
            );
        } catch (...) {ctx.handle_exception(); }
    });

    ProgressTracker progress_tracker("Chunks written", num_chunks);
    std::thread progress_thread([&](){
        try {
            progress_tracker.start(ctx, chunks_written);
        } catch (...) {ctx.handle_exception(); }
    });

    receiver_thread.join();
    decryption_thread.join();
    writer_thread.join();
    progress_thread.join();

    try{
        ctx.rethrow_if_exception();
    } catch (const std::exception& e) {
        std::cerr << "Transfer failed: " << e.what() << '\n';
        return;
    }

    auto end_time = std::chrono::system_clock::now();
    auto time_elapsed = std::chrono::duration<double>(end_time-start_time);
    auto seconds_elapsed = time_elapsed.count();
    
    std::cout << "Files received, transfer complete!" << std::endl;
    std::cout << "Time elapsed: " << seconds_elapsed << "s" << std::endl;
}