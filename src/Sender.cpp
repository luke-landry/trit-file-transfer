
#include <iostream>

#include "Sender.h"

Sender::Sender(const std::string ip_address_str, const unsigned short port): 
    io_context_(),
    receiver_endpoint_(asio::ip::address::from_string(ip_address_str), port),
    socket_(io_context_),
    stagingArea_() {};

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
    stagingArea_.stage_files();
}

/*
    Transfer request format:
    data [size]
    --------------------------
    number of files [4 bytes]
    total transfer size [8 bytes]
    uncompressed chunk size [4 bytes]
    uncompressed final chunk size [4 bytes]
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
bool Sender::send_transfer_request(){

    const auto& file_paths = stagingArea_.get_staged_file_paths();

    // Nothing to transfer
    if(file_paths.empty()){
        return false;
    }

    struct FileInfo {
        std::string path;
        uint64_t size;
    };

    std::vector<FileInfo> file_infos;
    std::vector<char> transfer_request;

    uint32_t num_files = file_paths.size();

    uint64_t total_transfer_size = 0;
    for(const auto& file_path : file_paths){
        uint64_t file_size = std::filesystem::file_size(file_path);
        file_infos.push_back({file_path, file_size});
        total_transfer_size += file_size;
    }

    uint32_t uncompressed_chunk_size = 4096;
    uint32_t final_uncompressed_chunk_size = total_transfer_size % uncompressed_chunk_size;

    return true;
   
}

void Sender::send_files(){

}
