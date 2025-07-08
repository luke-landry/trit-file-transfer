#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <variant>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"
#include "crypto.h"
#include "WorkerContext.h"

class EncryptionManager {
    public:

        // Encryptor constructor
        EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size, uint64_t num_chunks, crypto::Encryptor encryptor);

        // Decryptor constructor
        EncryptionManager(uint32_t chunk_size, uint32_t last_chunk_size, uint64_t num_chunks, crypto::Decryptor decryptor);

        void encrypt_chunks(
            WorkerContext& ctx,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
            std::atomic<bool>& input_done,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
            std::atomic<bool>& output_done);

        void decrypt_chunks(
            WorkerContext& ctx,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& input_queue,
            std::atomic<bool>& input_done,
            BoundedThreadSafeQueue<std::unique_ptr<Chunk>>& output_queue,
            std::atomic<bool>& output_done);

    private:
        const uint32_t chunk_size_;
        const uint32_t last_chunk_size_;
        const uint64_t num_chunks_;

        // EncryptionManager can be configured for either encryption or decryption
        std::variant<crypto::Encryptor, crypto::Decryptor> crypto_vnt_;

        crypto::Encryptor& get_encryptor();
        crypto::Decryptor& get_decryptor();
        std::unique_ptr<Chunk> encrypt_chunk(const Chunk& chunk);
        std::unique_ptr<Chunk> decrypt_chunk(const Chunk& chunk);
};

#endif