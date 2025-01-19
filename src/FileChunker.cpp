

#include "FileChunker.h"

FileChunker::FileChunker(const std::vector<std::filesystem::path>& file_paths, uint32_t chunk_size): file_paths_(file_paths), chunk_size_(chunk_size) {};

void FileChunker::start(BoundedThreadSafeQueue<Chunk>& out_queue){
    uint32_t sequence_counter = 0;

    for(const auto& file_path : file_paths_){
        uint64_t chunk_capacity_remaining = chunk_size_;
        uint64_t file_size = std::filesystem::file_size(file_path);

        if (file_size <= chunk_capacity_remaining){
            // read all file data into the chunk
        } else {
            // read portion of file data into current chunk
        }

        
    }
}