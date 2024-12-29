
#include "Chunk.h"

Chunk::Chunk(unsigned int sequence_num, std::size_t size): sequence_num_(sequence_num), data_(size) {};

unsigned int Chunk::sequence_num(){ return sequence_num_; }

const unsigned int Chunk::sequence_num() const{ return sequence_num_; }

uint8_t* Chunk::data() { return data_.data(); }

const uint8_t* Chunk::data() const { return data_.data(); }

std::size_t Chunk::size() const { return data_.size(); }