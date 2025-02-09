#ifndef TRANSFER_H
#define TRANSFER_H

#include <atomic>
#include <asio.hpp>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"

class Transfer {
    public:
        void send_chunks(
            asio::ip::tcp::socket& socket,
            BoundedThreadSafeQueue<Chunk>& in_queue,
            std::atomic<bool>& chunk_processing_done);

        void receive_chunks(asio::ip::tcp::socket& socket,
            BoundedThreadSafeQueue<Chunk>& out_queue,
            std::atomic<bool>& rx_done,
            uint32_t num_chunks);
};

#endif