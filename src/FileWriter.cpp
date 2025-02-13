
#include "FileWriter.h"
#include <fstream>

void FileWriter::start(const TransferRequest& transfer_request, BoundedThreadSafeQueue<Chunk>& in_queue, std::atomic<bool>& chunk_processing_done){
    uint32_t sequence_counter = 1;

    // Blocking thread until first chunk received
    Chunk chunk = in_queue.pop();

    uint32_t remaining_chunk_data = chunk.size();
    size_t chunk_offset = 0;

    for(const auto& file_info : transfer_request.get_file_infos()){
        std::filesystem::path abs_path = std::filesystem::current_path() / file_info.relative_path;
        uint64_t remaining_file_data = file_info.size;
        std::filesystem::path parent_path = std::filesystem::path(file_info.relative_path).parent_path();
        if(!parent_path.empty()){ std::filesystem::create_directories(parent_path); }
        std::ofstream file(file_info.relative_path, std::ios::binary);
        if(!file){ throw std::runtime_error("Failed to open file: " + abs_path.string()); }

        while(remaining_file_data > 0){
            const int64_t bytes_to_write = std::min<uint64_t>(remaining_file_data, remaining_chunk_data);
            file.write(reinterpret_cast<char*>(chunk.data() + chunk_offset), bytes_to_write);

            if(file.fail()){
                throw std::runtime_error("Failed to write to file " + abs_path.string());
            }
            
            chunk_offset += bytes_to_write;
            remaining_chunk_data -= bytes_to_write;
            remaining_file_data -= bytes_to_write;

            // Get next chunk once all data from this one is written
            if(remaining_chunk_data == 0){

                if(chunk_processing_done.load() && in_queue.empty()){
                    break;
                }

                chunk = in_queue.pop();
                remaining_chunk_data = chunk.size();
                chunk_offset = 0;
            }
        }
    }
}