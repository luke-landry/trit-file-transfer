
#include <iostream>

#include "Sender.h"

Sender::Sender(const std::string ip_address_str, const unsigned short port): 
    io_context_(),
    receiver_endpoint_(asio::ip::address::from_string(ip_address_str), port),
    socket_(io_context_) {};

void Sender::start_session(){

    connect_to_receiver();

    stage_files_for_transfer();

    send_transfer_request();

    send_files();
}

void Sender::connect_to_receiver(){
    const std::string receiver_address_str = receiver_endpoint_.address().to_string();
    unsigned short receiver_port = receiver_endpoint_.port();
    std::cout << "Attempting connection to " << receiver_address_str << " on port " << receiver_port << std::endl;

    socket_.connect(receiver_endpoint_);

    std::cout << "Connected" << std::endl;
}

void Sender::stage_files_for_transfer(){
    std::cout << "staging" << std::endl;
}

void Sender::send_transfer_request(){

}

void Sender::send_files(){
    
}
