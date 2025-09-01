#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <atomic>
#include <filesystem>
#include <vector>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"
#include "TransferRequest.h"
#include "WorkerContext.h"
#include "utils.h"

class FileManager {
public:
  void read_files_into_chunks(
      WorkerContext &ctx, const TransferRequest &transfer_request,
      BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &output_queue,
      std::atomic<bool> &output_done);

  void write_files_from_chunks(
      WorkerContext &ctx, const TransferRequest &transfer_request,
      BoundedThreadSafeQueue<std::unique_ptr<Chunk>> &input_queue,
      std::atomic<bool> &input_done, std::atomic<uint32_t> &chunks_written);
};

#endif