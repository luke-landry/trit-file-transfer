
#include "CompressionManager.h"

#include "zlib.h"
#include <stdexcept>
#include <iostream>

CompressionManager::CompressionManager(uint32_t chunk_size, uint32_t last_chunk_size) : chunk_size_(chunk_size), last_chunk_size_(last_chunk_size) {}

void CompressionManager::compress_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done) {

    while(true){
        auto chunk_ptr_opt = input_queue.try_pop();
        if(chunk_ptr_opt){
            auto& chunk_ptr = chunk_ptr_opt.value();
            auto compressed_chunk_ptr = compress_chunk(*chunk_ptr);

            // float percent_reduction = (1 - (compressed_chunk_ptr->size() * 1.0f / chunk_ptr->size()))*100;
            // std::cout << "Chunk #"<< chunk_ptr->sequence_num() << " compressed from " << chunk_ptr->size() << "B to "
            //       << compressed_chunk_ptr->size() << "B (" << percent_reduction << "% reduction)" << std::endl;

            output_queue.push(
                compressed_chunk_ptr->size() < chunk_ptr->size()
                ? std::move(compressed_chunk_ptr)
                : std::move(chunk_ptr)
            );
        } else if (input_done.load() && input_queue.empty()){
            break;
        }
    }
    output_done.store(true);
}

void CompressionManager::decompress_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){
        
    while(true){
        auto chunk_ptr_opt = input_queue.try_pop();
        if(chunk_ptr_opt){
            auto& chunk_ptr = chunk_ptr_opt.value();
            output_queue.push(
                chunk_ptr->compressed()
                ? std::move(decompress_chunk(*chunk_ptr))
                : std::move(chunk_ptr)
            );
        } else if (input_done.load() && input_queue.empty()){
            break;
        }
    }
    output_done.store(true);
}

std::unique_ptr<Chunk> CompressionManager::compress_chunk(const Chunk& chunk){
    if(chunk.size() > chunk_size_){
        throw std::runtime_error("Chunk size exceeded expected size of " + std::to_string(chunk_size_) + " bytes");
    }
    try {
        std::vector<uint8_t> compressed_data = compress_data(chunk.data(), chunk.size());

        

        return std::make_unique<Chunk>(chunk.sequence_num(), std::move(compressed_data), chunk.size());
    } catch (const std::exception& e){
        throw std::runtime_error(
            "Error when compressing chunk #" +
            std::to_string(chunk.sequence_num()) + 
            ": " + e.what());
    }
}

std::unique_ptr<Chunk> CompressionManager::decompress_chunk(const Chunk& chunk){
    try {
        std::vector<uint8_t> decompressed_data = decompress_data(chunk.data(), chunk.size(), chunk.original_size());

        // float percent_reduction = (1 - (chunk.size() * 1.0f / decompressed_data.size()))*100;
        // std::cout << "Chunk #"<< chunk.sequence_num() << " decompressed from " << chunk.size() << "B to "
        //           << decompressed_data.size() << "B (" << percent_reduction << "% reduction)" << std::endl;

        return std::make_unique<Chunk>(chunk.sequence_num(), std::move(decompressed_data));
    } catch (const std::exception& e){
        throw std::runtime_error(
            "Error when decompressing chunk #" +
            std::to_string(chunk.sequence_num()) + 
            ": " + e.what());
    }
}

std::vector<uint8_t> CompressionManager::compress_data(const uint8_t* data, uint16_t data_size) {
    auto compressed_size_bound = compressBound(data_size);
    std::vector<uint8_t> compressed_data(compressed_size_bound);
    auto compressed_size = compressed_size_bound;
    int ret = compress(compressed_data.data(), &compressed_size, data, data_size);
    if(ret != Z_OK){ throw std::runtime_error("zlib error code: " + std::to_string(ret)); }
    compressed_data.resize(compressed_size);
    return compressed_data;
}

std::vector<uint8_t> CompressionManager::decompress_data(const uint8_t* compressed_data, uint16_t compressed_data_size, uint16_t original_size) {
    if(compressed_data_size == 0){ return std::vector<uint8_t>(); }
    std::vector<uint8_t> data(original_size);
    uLongf data_size = data.size();
    int ret = uncompress(data.data(), &data_size, compressed_data, compressed_data_size);
    if(ret != Z_OK){ throw std::runtime_error("zlib error code: " + std::to_string(ret));}
    if(data_size > original_size) { throw std::runtime_error("decompression buffer overflow"); }
    return data;
}