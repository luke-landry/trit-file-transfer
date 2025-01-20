
#include <iostream>
#include <algorithm>

#include "Sender.h"
#include "utils.h"
#include "FileChunker.h"

Sender::Sender(const std::string ip_address_str, const unsigned short port): 
    io_context_(),
    receiver_endpoint_(asio::ip::address::from_string(ip_address_str), port),
    socket_(io_context_),
    stagingArea_() {};

void Sender::start_session(){

    connect_to_receiver();

    stage_files_for_transfer();
    TransferRequest transfer_request = send_transfer_request();

    while(!transfer_request_accepted()){
        stage_files_for_transfer();
        transfer_request = send_transfer_request();
    }

    send_files(transfer_request);
}

void Sender::connect_to_receiver(){
    const std::string receiver_address_str = receiver_endpoint_.address().to_string();
    unsigned short receiver_port = receiver_endpoint_.port();
    std::cout << "Attempting connection to " << receiver_address_str << " on port " << receiver_port << std::endl;

    socket_.connect(receiver_endpoint_);

    std::cout << "Connected" << std::endl;
}

void Sender::stage_files_for_transfer(){
    stagingArea_.stage_files();
}

TransferRequest Sender::send_transfer_request(){

    TransferRequest transfer_request = TransferRequest::from_file_paths(stagingArea_.get_staged_file_paths());
    std::vector<uint8_t> transfer_request_buffer = transfer_request.serialize();
    uint64_t transfer_request_buffer_size = transfer_request_buffer.size();

    transfer_request.print();

    std::cout << "Serialized data (" << transfer_request_buffer_size << " bytes):" << std::endl;
    utils::print_buffer(transfer_request_buffer);
    
    // Write size, then transfer request to receiver
    std::cout << "Sending transfer request..." << std::endl;
    asio::write(socket_, asio::buffer(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size)));
    asio::write(socket_, asio::buffer(transfer_request_buffer));

    return transfer_request;
}

bool Sender::transfer_request_accepted(){
    uint8_t request_accepted_byte;

    asio::read(socket_, asio::buffer(&request_accepted_byte, sizeof(request_accepted_byte)));

    bool request_accepted = request_accepted_byte;

    std::cout << ((request_accepted ? "Transfer accepted" : "Transfer denied")) << std::endl;

    return request_accepted;
}

void Sender::send_files(const TransferRequest& transfer_request){
    std::cout << "Sending files..." << std::endl;

    constexpr int QUEUE_CAPACITY = 50;

    BoundedThreadSafeQueue<Chunk> uncompressed_chunk_queue(QUEUE_CAPACITY);

    FileChunker file_chunker;

    std::thread chunker_thread([&](){
        file_chunker.start(transfer_request, uncompressed_chunk_queue);
    });

    // TODO implement thread safe logging


    // TODO temporary mechanism to keep main thread running
    while(true){}
}
