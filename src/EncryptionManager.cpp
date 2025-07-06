#include "EncryptionManager.h"

#include <stdexcept>
#include <iostream>

namespace {

template <typename Transform>
void process_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done,
    Transform&& transform)
{
    while (true) {
        auto chunk_ptr_opt = input_queue.try_pop();
        if (chunk_ptr_opt) {
            auto chunk_ptr = std::move(*chunk_ptr_opt);
            auto result = transform(*chunk_ptr);
            output_queue.push(std::move(result));
        } else if (input_done.load() && input_queue.empty()) {
            break;
        }
    }
    output_done.store(true);
}

} // anonymous namespace

EncryptionManager::EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size, uint64_t num_chunks, crypto::Encryptor encryptor)
    : chunk_size_(chunk_size), last_chunk_size_(last_chunk_size), num_chunks_(num_chunks),
      crypto_vnt_(std::move(encryptor)) {}

EncryptionManager::EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size, uint64_t num_chunks, crypto::Decryptor decryptor)
    : chunk_size_(chunk_size), last_chunk_size_(last_chunk_size), num_chunks_(num_chunks),
      crypto_vnt_(std::move(decryptor)) {}

crypto::Encryptor& EncryptionManager::get_encryptor(){
    if(std::holds_alternative<crypto::Encryptor>(crypto_vnt_)){
        return std::get<crypto::Encryptor>(crypto_vnt_);
    } else {
        throw std::logic_error("EncryptionManager was not configured for encryption");
    }
}

crypto::Decryptor& EncryptionManager::get_decryptor(){
    if(std::holds_alternative<crypto::Decryptor>(crypto_vnt_)){
        return std::get<crypto::Decryptor>(crypto_vnt_);
    } else {
        throw std::logic_error("EncryptionManager was not configured for decryption");
    }
}

void EncryptionManager::encrypt_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){

    process_chunks(input_queue, input_done, output_queue, output_done,
        [this](Chunk& c) { return encrypt_chunk(c); });
}

void EncryptionManager::decrypt_chunks(
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
    std::atomic<bool>& input_done,
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
    std::atomic<bool>& output_done){

    process_chunks(input_queue, input_done, output_queue, output_done,
        [this](Chunk& c) { return decrypt_chunk(c); });
}

std::unique_ptr<Chunk> EncryptionManager::encrypt_chunk(const Chunk& chunk){
    if (chunk.sequence_num() == 0 || chunk.sequence_num() > num_chunks_) {
        throw std::runtime_error("EncryptionManager: invalid chunk sequence number: " + std::to_string(chunk.sequence_num()));
    }

    if(chunk.size() > chunk_size_){
        throw std::runtime_error("Chunk size exceeded expected size of " + std::to_string(chunk_size_) + " bytes");
    }

    std::vector<uint8_t> encrypted_data(chunk.size() + crypto::ENCRYPTION_ADDITIONAL_BYTES);
    std::size_t out_len = 0;
    const bool is_final = chunk.sequence_num() == num_chunks_;
    get_encryptor().encrypt(chunk.data(), chunk.size(), encrypted_data.data(), &out_len, is_final);
    if (out_len != encrypted_data.size()) {
        throw std::runtime_error("EncryptionManager: unexpected encrypted output size");
    }
    return std::make_unique<Chunk>(chunk.sequence_num(), std::move(encrypted_data), chunk.size());
}

std::unique_ptr<Chunk> EncryptionManager::decrypt_chunk(const Chunk& chunk){
    if (chunk.sequence_num() == 0 || chunk.sequence_num() > num_chunks_) {
        throw std::runtime_error("EncryptionManager: invalid chunk sequence number: " + std::to_string(chunk.sequence_num()));
    }

    std::vector<uint8_t> decrypted_data(chunk.size() - crypto::ENCRYPTION_ADDITIONAL_BYTES);
    std::size_t out_len = 0;
    bool is_final = false;
    get_decryptor().decrypt(chunk.data(), chunk.size(), decrypted_data.data(), &out_len, is_final);
    if (out_len != decrypted_data.size()) {
        throw std::runtime_error("EncryptionManager: unexpected decrypted output size");
    }
    if (is_final != (chunk.sequence_num() == num_chunks_)){
        throw std::runtime_error("EncryptionManager: final chunk decryption mismatch");
    }   
    return std::make_unique<Chunk>(chunk.sequence_num(), std::move(decrypted_data));
}