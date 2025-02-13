#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <atomic>

#include "TransferRequest.h"
#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"

class FileWriter {
    public:
        void start(const TransferRequest& transfer_request, BoundedThreadSafeQueue<Chunk>& in_queue, std::atomic<bool>& chunk_processing_done);
};

#endif