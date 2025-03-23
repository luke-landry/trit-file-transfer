#ifndef CHUNK_H
#define CHUNK_H

#include <cstdint>
#include <vector>

class Chunk {
    public:
        Chunk( uint64_t sequence_num, std::vector<uint8_t>&& data);

        // Constructor for compressed chunks, where data.size != original size
        Chunk(
            uint64_t sequence_num,
            std::vector<uint8_t>&& data,
            uint16_t original_size_
        );

        // Chunks should never be copied, only moved
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk(Chunk&& other) noexcept = default;
        Chunk& operator=(Chunk&& other) noexcept = default;

        uint64_t sequence_num();
        const uint64_t sequence_num() const;

        const uint8_t* data() const;

        uint16_t size() const;

        bool compressed() const;

        uint16_t original_size() const;

    private:
        const uint64_t sequence_num_;
        const bool compressed_;

        // original_size_ must be declared before data_ so that it is
        // initialized with the data.size() argument in the constructor 
        // before data_ is initialized, since data_(std::move(data))
        uint16_t original_size_;
        std::vector<uint8_t> data_;
        
};

#endif