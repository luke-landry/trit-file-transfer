
#include <iostream>
#include <algorithm>
#include <thread>

#include "Sender.h"
#include "utils.h"
#include "FileManager.h"
#include "TransferManager.h"
#include "ProgressTracker.h"

Sender::Sender(const std::string ip_address_str, const uint16_t port): 
    io_context_(),
    receiver_endpoint_(asio::ip::address::from_string(ip_address_str), port),
    socket_(io_context_),
    stagingArea_() {};

void Sender::start_session(){
    connect_to_receiver();

    TransferRequest transfer_request = stage_files_for_transfer();
    while(!send_transfer_request(transfer_request)){
        transfer_request = stage_files_for_transfer();
    }

    send_files(transfer_request);
}

void Sender::connect_to_receiver(){
    const std::string receiver_address_str = receiver_endpoint_.address().to_string();
    uint16_t receiver_port = receiver_endpoint_.port();
    socket_.connect(receiver_endpoint_);
    std::cout << "Connected to " << receiver_address_str << " on port " << receiver_port << std::endl;
}

TransferRequest Sender::stage_files_for_transfer(){
    stagingArea_.stage_files();
    return TransferRequest::from_file_paths(stagingArea_.get_staged_file_paths());
}

bool Sender::send_transfer_request(const TransferRequest& transfer_request){
    // Serialize and then send transfer request to receivevr
    std::vector<uint8_t> transfer_request_buffer = transfer_request.serialize();
    uint64_t transfer_request_buffer_size = transfer_request_buffer.size();
    asio::write(socket_, asio::buffer(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size)));
    asio::write(socket_, asio::buffer(transfer_request_buffer));
    std::cout << "Transfer request sent" << std::endl;

    // Get accept/deny response
    uint8_t request_accepted_byte;
    asio::read(socket_, asio::buffer(&request_accepted_byte, sizeof(request_accepted_byte)));
    bool request_accepted = request_accepted_byte;
    std::cout << (request_accepted ? "Transfer accepted" : "Transfer denied") << std::endl;
    return request_accepted;
}

void Sender::send_files(const TransferRequest& transfer_request){
    std::cout << "Sending files..." << std::endl;

    constexpr int QUEUE_CAPACITY = 50;

    // Completion flags
    std::atomic<bool> file_chunking_done(false);

    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> uncompressed_chunk_queue(QUEUE_CAPACITY);

    FileManager file_chunker;
    std::thread chunker_thread([&](){
        file_chunker.read_files_into_chunks(transfer_request, uncompressed_chunk_queue, file_chunking_done);
    });

    // TODO compressor for chunks

    // TODO encryptor for chunks

    TransferManager chunk_sender;
    std::thread transmission_thread([&](){
        chunk_sender.send_chunks(socket_, uncompressed_chunk_queue, file_chunking_done);
    });

    chunker_thread.join();
    transmission_thread.join();

    std::cout << "Files sent, transfer complete!" << std::endl;
}
