#ifndef TRANSFER_H
#define TRANSFER_H

#include <atomic>
#include <asio.hpp>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"

class TransferManager {
    public:
        void send_chunks(
            asio::ip::tcp::socket& socket,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& in_queue,
            std::atomic<bool>& chunk_processing_done,
            std::atomic<uint32_t>& chunks_sent);

        void receive_chunks(asio::ip::tcp::socket& socket,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& out_queue,
            std::atomic<bool>& rx_done,
            uint32_t num_chunks);
};

#endif