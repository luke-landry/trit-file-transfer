
#include <stdexcept>
#include <iostream>

#include "TransferManager.h"
#include "utils.h"

/*
Chunk transfer protocol:
[2 bytes] chunk size
[variable] chunk data
*/

void TransferManager::send_chunks(
    asio::ip::tcp::socket& socket,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& in_queue,
    std::atomic<bool>& chunk_processing_done,
    std::atomic<uint32_t>& chunks_sent){

    while(true){
        std::optional<std::unique_ptr<Chunk>> chunk_ptr_opt = in_queue.try_pop();
        if(chunk_ptr_opt){
            std::unique_ptr<Chunk> chunk_ptr = std::move(chunk_ptr_opt.value());
            if(chunk_ptr->size() > UINT16_MAX){
                throw std::runtime_error("Chunk exceeded maximum size of " + std::to_string(UINT16_MAX) + " bytes");
            }
            uint16_t chunk_size = static_cast<uint16_t>(chunk_ptr->size());
            asio::write(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
            asio::write(socket, asio::buffer(chunk_ptr->data(), chunk_size));
            // std::cout << "Sent chunk (seq#) " << chunk_ptr->sequence_num() << " (" << chunk_ptr->size() << " B)" << std::endl;
            chunks_sent++;
        } else if (chunk_processing_done.load()){
            break;
        }
    }
}

void TransferManager::receive_chunks(
    asio::ip::tcp::socket& socket,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& out_queue,
    std::atomic<bool>& chunk_reception_done, uint32_t num_chunks){

    uint32_t sequence_counter = 1;
    while(sequence_counter <= num_chunks){
        uint16_t chunk_size;
        asio::read(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
        std::vector<uint8_t> buffer(chunk_size);
        asio::read(socket, asio::buffer(buffer.data(), chunk_size));
        out_queue.push(std::make_unique<Chunk>(sequence_counter++, std::move(buffer)));
    }

    chunk_reception_done.store(true);
}