#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <vector>
#include <filesystem>
#include <atomic>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"
#include "utils.h"
#include "TransferRequest.h"

class FileManager {
    public:
        void read_files_into_chunks(const TransferRequest& transfer_request, BoundedThreadSafeQueue<Chunk>& out_queue, std::atomic<bool>& file_chunking_done);
        void write_files_from_chunks(const TransferRequest& transfer_request, BoundedThreadSafeQueue<Chunk>& in_queue, std::atomic<bool>& chunk_processing_done);
};

#endif