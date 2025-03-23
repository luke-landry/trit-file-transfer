
#include <stdexcept>
#include <iostream>

#include "TransferManager.h"
#include "utils.h"

/*
Chunk transfer protocol:
sequence number     [8 bytes]
compressed flag     [1 byte] 
original size       [2 bytes]
chunk size          [2 bytes] 
chunk data          [<= 65535 bytes] 
*/

void TransferManager::send_chunks(
    asio::ip::tcp::socket& socket,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    std::atomic<uint32_t>& chunks_sent){

    while(true){
        auto chunk_ptr_opt = input_queue.try_pop();
        if(chunk_ptr_opt){
            const Chunk& chunk = *chunk_ptr_opt.value();
            if(chunk.size() > UINT16_MAX){
                throw std::runtime_error("Chunk exceeded maximum size of " + std::to_string(UINT16_MAX) + " bytes");
            }
            
            uint64_t sequence_num = chunk.sequence_num();
            asio::write(socket, asio::buffer(&sequence_num, sizeof(sequence_num)));

            uint8_t compressed_flag = static_cast<uint8_t>(chunk.compressed());
            asio::write(socket, asio::buffer(&compressed_flag, sizeof(compressed_flag)));

            uint16_t original_size = chunk.original_size();
            asio::write(socket, asio::buffer(&original_size, sizeof(original_size)));

            uint16_t chunk_size = chunk.size();
            asio::write(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
            asio::write(socket, asio::buffer(chunk.data(), chunk_size));

            // std::cout << "Sent chunk (seq#) " << chunk_ptr->sequence_num() << " (" << chunk_ptr->size() << " B)" << std::endl;
            chunks_sent++;
        } else if (input_done.load() && input_queue.empty()){
            break;
        }
    }
}

void TransferManager::receive_chunks(
    asio::ip::tcp::socket& socket,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done,
    uint32_t num_chunks){

    for(uint32_t i = 0; i < num_chunks; ++i){
        uint64_t sequence_num;
        asio::read(socket, asio::buffer(&sequence_num, sizeof(sequence_num)));

        uint8_t compressed_flag;
        asio::read(socket, asio::buffer(&compressed_flag, sizeof(compressed_flag)));
        bool compressed = static_cast<bool>(compressed_flag);

        uint16_t original_size;
        asio::read(socket, asio::buffer(&original_size, sizeof(original_size)));

        uint16_t chunk_size;
        asio::read(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
        std::vector<uint8_t> buffer(chunk_size);
        asio::read(socket, asio::buffer(buffer.data(), chunk_size));

        output_queue.push(
            compressed ? std::make_unique<Chunk>(sequence_num, std::move(buffer), original_size)
                       : std::make_unique<Chunk>(sequence_num, std::move(buffer))
        );
    }

    output_done.store(true);
}