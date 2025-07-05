#include "EncryptionManager.h"

#include <stdexcept>
#include <iostream>

EncryptionManager::EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size)
    : chunk_size_(chunk_size), last_chunk_size_(last_chunk_size) {}

void EncryptionManager::encrypt_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){

    while(true){
        auto chunk_ptr_opt = input_queue.try_pop();
        if(chunk_ptr_opt){
            auto& chunk_ptr = chunk_ptr_opt.value();
            auto encrypted_chunk_ptr = encrypt_chunk(*chunk_ptr);
            output_queue.push(std::move(encrypted_chunk_ptr));
        } else if (input_done.load() && input_queue.empty()) {
            break;
        }
    }
    output_done.store(true);
}

void EncryptionManager::decrypt_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){

    while(true){
        auto chunk_ptr_opt = input_queue.try_pop();
        if(chunk_ptr_opt){
            auto& chunk_ptr = chunk_ptr_opt.value();
            output_queue.push(decrypt_chunk(*chunk_ptr));
        } else if (input_done.load() && input_queue.empty()) {
            break;
        }
    }
    output_done.store(true);
}

std::unique_ptr<Chunk> EncryptionManager::encrypt_chunk(const Chunk& chunk){
    if(chunk.size() > chunk_size_){
        throw std::runtime_error("Chunk size exceeded expected size of " + std::to_string(chunk_size_) + " bytes");
    }
    std::vector<uint8_t> encrypted_data = encrypt_data(chunk.data(), chunk.size());
    return std::make_unique<Chunk>(chunk.sequence_num(), std::move(encrypted_data), chunk.size());
}

std::unique_ptr<Chunk> EncryptionManager::decrypt_chunk(const Chunk& chunk){
    std::vector<uint8_t> decrypted_data = decrypt_data(chunk.data(), chunk.size());
    return std::make_unique<Chunk>(chunk.sequence_num(), std::move(decrypted_data));
}

std::vector<uint8_t> EncryptionManager::encrypt_data(const uint8_t* data, uint16_t data_size){
    // TODO: implement encryption
    (void)data;
    (void)data_size;
    return {};
}

std::vector<uint8_t> EncryptionManager::decrypt_data(const uint8_t* encrypted_data, uint16_t data_size){
    // TODO: implement decryption
    (void)encrypted_data;
    (void)data_size;
    return {};
}