#ifndef TRANSFER_H
#define TRANSFER_H

#include <atomic>

#include "BoundedThreadSafeQueue.h"
#include "TcpSocket.h"
#include "Chunk.h"
#include "WorkerContext.h"

class TransferManager {
    public:
        void send_chunks(
            WorkerContext& ctx,    
            TcpSocket& socket,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
            std::atomic<bool>& input_done,
            std::atomic<uint32_t>& chunks_sent);

        void receive_chunks(
            WorkerContext& ctx,
            TcpSocket& socket,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
            std::atomic<bool>& output_done,
            uint32_t num_chunks);
};

#endif