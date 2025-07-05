#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include <vector>
#include <thread>
#include <memory>
#include <atomic>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"

class EncryptionManager {
    public:
        EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size);

        void encrypt_chunks(
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
            std::atomic<bool>& input_done,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
            std::atomic<bool>& output_done);

        void decrypt_chunks(
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
            std::atomic<bool>& input_done,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
            std::atomic<bool>& output_done);

        std::unique_ptr<Chunk> encrypt_chunk(const Chunk& chunk);
        std::unique_ptr<Chunk> decrypt_chunk(const Chunk& chunk);
        std::vector<uint8_t> encrypt_data(const uint8_t* data, uint16_t data_size);
        std::vector<uint8_t> decrypt_data(const uint8_t* encrypted_data, uint16_t data_size);

    private:
        const uint32_t chunk_size_;
        const uint32_t last_chunk_size_;
};

#endif