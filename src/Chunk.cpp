
#include "Chunk.h"

Chunk::Chunk(
    uint64_t sequence_num,
    std::vector<uint8_t>&& data):
    sequence_num_(sequence_num),
    compressed_(false),
    original_size_(data.size()),
    data_(std::move(data)) {};

Chunk::Chunk(
    uint64_t sequence_num,
    std::vector<uint8_t>&& data,
    uint16_t original_size):
    sequence_num_(sequence_num),
    compressed_(true),
    original_size_(original_size),
    data_(std::move(data)) {};

uint64_t Chunk::sequence_num(){ return sequence_num_; }

const uint64_t Chunk::sequence_num() const{ return sequence_num_; }

const uint8_t* Chunk::data() const { return data_.data(); }

uint16_t Chunk::size() const { return static_cast<uint16_t>(data_.size()); }

bool Chunk::compressed() const { return compressed_; }

uint16_t Chunk::original_size() const { return original_size_; }