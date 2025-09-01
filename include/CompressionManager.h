#ifndef COMPRESSION_MANAGER_H
#define COMPRESSION_MANAGER_H

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"

class CompressionManager {
public:
  CompressionManager(uint32_t chunk_size, uint32_t last_chunk_size);

  void
  compress_chunks(BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &input_queue,
                  std::atomic<bool> &input_done,
                  BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &output_queue,
                  std::atomic<bool> &output_done);

  void decompress_chunks(
      BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &input_queue,
      std::atomic<bool> &input_done,
      BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &output_queue,
      std::atomic<bool> &output_done);

  std::unique_ptr<Chunk> compress_chunk(const Chunk &chunk);
  std::unique_ptr<Chunk> decompress_chunk(const Chunk &chunk);
  std::vector<uint8_t> compress_data(const uint8_t *data, uint16_t data_size);
  std::vector<uint8_t> decompress_data(const uint8_t *compressed_data,
                                       uint16_t compressed_data_size,
                                       uint16_t original_size);

private:
  const uint32_t chunk_size_;
  const uint32_t last_chunk_size_;
};

#endif