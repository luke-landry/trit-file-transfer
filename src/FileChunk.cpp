
#include "FileChunk.h"

FileChunk::FileChunk(std::size_t size): data_(size) {};

char* FileChunk::data(){ return data_.data(); }

const char* FileChunk::data() const { return data_.data(); }

std::size_t FileChunk::size() const { return data_.size(); }