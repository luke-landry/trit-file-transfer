
#include <stdexcept>
#include <iostream>

#include "Transfer.h"
#include "utils.h"

/*
Chunk transfer protocol:
[2 bytes] chunk size
[variable] chunk data
*/

void Transfer::send_chunks(asio::ip::tcp::socket& socket, BoundedThreadSafeQueue<Chunk>& in_queue, std::atomic<bool>& chunk_processing_done){
    while(true){
        std::optional<Chunk> chunk_opt = in_queue.try_pop();
        if(chunk_opt){
            Chunk chunk = std::move(chunk_opt.value());
            
            if(chunk.size() > UINT16_MAX){
                throw std::runtime_error("Chunk exceeded maximum size of " + std::to_string(UINT16_MAX) + " bytes");
            }

            uint16_t chunk_size = static_cast<uint16_t>(chunk.size());

            asio::write(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
            asio::write(socket, asio::buffer(chunk.data(), chunk_size));

            std::cout << "Sending chunk " << chunk.sequence_num() << std::endl;
            utils::print_buffer(std::vector<uint8_t>(chunk.data(), chunk.data() + chunk.size()));
        } else if (chunk_processing_done.load()){
            break;
        }
    }
}

void Transfer::receive_chunks(asio::ip::tcp::socket& socket, BoundedThreadSafeQueue<Chunk>& out_queue, std::atomic<bool>& chunk_reception_done, uint32_t num_chunks){
    uint32_t sequence_counter = 1;
    while(sequence_counter <= num_chunks){
        uint16_t chunk_size;
        asio::read(socket, asio::buffer(&chunk_size, sizeof(chunk_size)));
        std::vector<uint8_t> buffer(chunk_size);
        asio::read(socket, asio::buffer(buffer.data(), chunk_size));

        // std::cout << "Received chunk " << sequence_counter << " of " << num_chunks << " [" << chunk_size << "] bytes" << std::endl;
        // utils::print_buffer(buffer);

        out_queue.emplace(sequence_counter, std::move(buffer));

        sequence_counter++;
    }

    chunk_reception_done.store(true);
}