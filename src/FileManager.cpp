
#include "FileManager.h"

#include <fstream>

void FileManager::read_files_into_chunks(
    WorkerContext& ctx,
    const TransferRequest& transfer_request,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){
    
    uint32_t sequence_counter = 1;
    uint32_t remaining_buffer_capacity = transfer_request.get_chunk_size();
    std::vector<uint8_t> buffer(transfer_request.get_chunk_size());

    for(const auto& file_info : transfer_request.get_file_infos()){
        if(ctx.should_abort()){ return; }

        std::filesystem::path file_path = std::filesystem::current_path() / file_info.relative_path;
        uint64_t file_size = std::filesystem::file_size(file_path);
        
        if(file_size != file_info.size){
            throw std::runtime_error("\nFile size mismatch: expected " + std::to_string(file_info.size) + " bytes, file size is " + std::to_string(file_size) + " bytes");
        }

        uint64_t remaining_file_data = file_size;
        std::ifstream file(file_path, std::ios::binary);
        if(!file){
            throw std::runtime_error("\nFailed to open file: " + file_path.string());
        }

        // Chunks are a specific size, so multiple smaller files could be contained in one chunk
        // and larger files could be made up of multiple chunks
        while(remaining_file_data > 0){
            if(ctx.should_abort()){ return; }

            const uint64_t bytes_to_read = std::min<uint64_t>(remaining_file_data, remaining_buffer_capacity);
            file.read(reinterpret_cast<char*>(buffer.data() + (buffer.size() - remaining_buffer_capacity)), bytes_to_read);

            auto bytes_read = file.gcount();
            if(bytes_read != bytes_to_read){
                throw std::runtime_error("\nDid not read expected number of bytes");
            }

            remaining_buffer_capacity -= bytes_read;
            remaining_file_data -= bytes_read;

            // Allocate new chunk buffer if the current one is filled up
            if(remaining_buffer_capacity == 0){
                output_queue.push(std::make_unique<Chunk>(sequence_counter++, std::move(buffer)));

                // Final chunk size should only be used on the last chunk when the calculated chunk size is not 0.
                // If it is 0, then this may indicate that the transfer size is perfectly divisble by the chunk size,
                // in which case the final chunk should be regularily sized, not 0 bytes.
                const bool use_final_chunk_size = (sequence_counter == transfer_request.get_num_chunks()) && (transfer_request.get_final_chunk_size() != 0);
                const uint32_t new_buffer_size = use_final_chunk_size ? transfer_request.get_final_chunk_size() : transfer_request.get_chunk_size();
                buffer.resize(new_buffer_size);
                remaining_buffer_capacity = new_buffer_size;
            }
        }
    }
    
    output_done.store(true);
}


void FileManager::write_files_from_chunks(
    WorkerContext& ctx,
    const TransferRequest& transfer_request,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    std::atomic<uint32_t>& chunks_written){

    uint32_t sequence_counter = 1;

    // Blocking thread until first chunk received
    std::unique_ptr<Chunk> chunk_ptr = input_queue.pop();

    uint32_t remaining_chunk_data = chunk_ptr->size();
    size_t chunk_offset = 0;

    for(const auto& file_info : transfer_request.get_file_infos()){
        std::filesystem::path abs_path = std::filesystem::current_path() / file_info.relative_path;
        uint64_t remaining_file_data = file_info.size;
        std::filesystem::path parent_path = std::filesystem::path(file_info.relative_path).parent_path();
        if(!parent_path.empty()){ std::filesystem::create_directories(parent_path); }
        std::ofstream file(file_info.relative_path, std::ios::binary);
        if(!file){ throw std::runtime_error("Failed to open file: " + abs_path.string()); }

        while(remaining_file_data > 0){
            if(ctx.should_abort()) { return; }
            
            const int64_t bytes_to_write = std::min<uint64_t>(remaining_file_data, remaining_chunk_data);
            file.write(reinterpret_cast<const char*>(chunk_ptr->data() + chunk_offset), bytes_to_write);

            if(file.fail()){
                throw std::runtime_error("Failed to write to file " + abs_path.string());
            }
            
            chunk_offset += bytes_to_write;
            remaining_chunk_data -= bytes_to_write;
            remaining_file_data -= bytes_to_write;

            // Get next chunk once all data from this one is written
            if(remaining_chunk_data == 0){
                // std::cout << "Wrote chunk (seq#) " << chunk_ptr->sequence_num() << " (" << chunk_ptr->size() << " B)" << std::endl;
                chunks_written++;
                
                if(input_done.load() && input_queue.empty()){
                    break;
                }

                chunk_ptr = input_queue.pop();
                remaining_chunk_data = chunk_ptr->size();
                chunk_offset = 0;
            }
        }
    }
}