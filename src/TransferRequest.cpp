
#include "TransferRequest.h"
#include "utils.h"

TransferRequest::TransferRequest(uint32_t num_files, uint64_t transfer_size,
                                 uint32_t uncompressed_chunk_size,
                                 uint32_t uncompressed_final_chunk_size,
                                 uint32_t num_chunks,
                                 std::vector<FileInfo> file_infos)
    : num_files_(num_files), transfer_size_(transfer_size),
      uncompressed_chunk_size_(uncompressed_chunk_size),
      uncompressed_final_chunk_size_(uncompressed_final_chunk_size),
      num_chunks_(num_chunks), file_infos_(file_infos) {};

TransferRequest TransferRequest::from_file_paths(
    const std::unordered_set<std::filesystem::path> &file_paths) {

  const uint32_t num_files = file_paths.size();

  if (num_files == 0) {
    throw std::runtime_error(
        "Cannot create transfer request from empty set of file paths");
  }

  uint64_t transfer_size = 0;

  // Determine file sizes and store in struct, while also calculating total size
  // Only store the relative generic paths from staging root directory because
  // absolute paths should not be sent
  std::vector<FileInfo> file_infos;
  for (const auto &file_path : file_paths) {
    auto relative_generic_path =
        std::filesystem::relative(file_path, std::filesystem::current_path())
            .generic_string();
    auto file_size = std::filesystem::file_size(file_path);
    file_infos.emplace_back(relative_generic_path, file_size);
    transfer_size += file_size;
  }

  if (transfer_size == 0) {
    throw std::runtime_error("Cannot create transfer request because size of "
                             "transfer would be 0 bytes");
  }

  // If total size is less than the max chunk size, use the total transfer size
  // so that the transfer is exactly one chunk Otherwise, use the max chunk
  // size, and the transfer will be divided into multiple chunks
  uint32_t uncompressed_chunk_size = std::min(
      transfer_size, static_cast<uint64_t>(utils::MAX_UNCOMPRESSED_CHUNK_SIZE));

  // Last chunk contains remaining data, which is variable
  uint32_t uncompressed_last_chunk_size =
      transfer_size % uncompressed_chunk_size;

  // Yields number of chunks, and results in extra chunk if total size not
  // perfectly divisible
  uint32_t num_chunks =
      (transfer_size + (uncompressed_chunk_size - 1)) / uncompressed_chunk_size;

  return TransferRequest(num_files, transfer_size, uncompressed_chunk_size,
                         uncompressed_last_chunk_size, num_chunks, file_infos);
}

/*
    This method deserializes the file transfer request based on the following
   format
    --------------------------
    number of files [4 bytes]
    total transfer size [8 bytes]
    uncompressed chunk size [4 bytes]
    uncompressed last chunk size [4 bytes]
    num chunks [4 bytes]
    file1 path length [2 bytes]
    file1 path [variable]
    file1 size [8 bytes]
    file2 path length [2 bytes]
    file2 path [variable]
    file2 size [8 bytes]
    ...
    fileN path length [2 bytes]
    fileN path [variable]
    fileN size [8 bytes]
*/
TransferRequest
TransferRequest::deserialize(const std::vector<uint8_t> &buffer) {

  auto it = buffer.begin();
  auto end = buffer.end();

  uint32_t num_files;
  it = utils::deserialize(it, end, num_files);

  uint64_t transfer_size;
  it = utils::deserialize(it, end, transfer_size);

  uint32_t uncompressed_chunk_size;
  it = utils::deserialize(it, end, uncompressed_chunk_size);

  uint32_t uncompressed_last_chunk_size;
  it = utils::deserialize(it, end, uncompressed_last_chunk_size);

  uint32_t num_chunks;
  it = utils::deserialize(it, end, num_chunks);

  // Serialize file size and generic path strings
  std::vector<FileInfo> file_infos;
  for (uint32_t i = 0; i < num_files; ++i) {
    uint16_t path_length;
    it = utils::deserialize(it, end, path_length);

    std::string path(path_length, '\0');
    it = utils::deserialize(it, end, path);

    uint64_t size;
    it = utils::deserialize(it, end, size);

    file_infos.emplace_back(path, size);
  }

  return TransferRequest(num_files, transfer_size, uncompressed_chunk_size,
                         uncompressed_last_chunk_size, num_chunks, file_infos);
}

/*
    This method serializes the file transfer request based on  the following
   format
    --------------------------
    [4 bytes] number of files
    [8 bytes] total transfer size
    [4 bytes] uncompressed chunk size
    [4 bytes] uncompressed last chunk size
    [4 bytes] num chunks
    [2 bytes] file1 path length
    [variable] file1 path
    [8 bytes] file1 size
    [2 bytes] file2 path length
    [variable] file2 path
    [8 bytes] file2 size
    ...
    [2 bytes] fileN path length
    [variable] fileN path
    [8 bytes] fileN size
*/
std::vector<uint8_t> TransferRequest::serialize() const {

  std::vector<uint8_t> transfer_request_buffer;

  // Append serialized fixed size metadata to transfer request
  utils::serialize(num_files_, transfer_request_buffer);
  utils::serialize(transfer_size_, transfer_request_buffer);
  utils::serialize(uncompressed_chunk_size_, transfer_request_buffer);
  utils::serialize(uncompressed_final_chunk_size_, transfer_request_buffer);
  utils::serialize(num_chunks_, transfer_request_buffer);

  // Serialize file size and generic path strings
  for (const auto &file_info : file_infos_) {
    if (file_info.relative_path.size() >
        std::numeric_limits<uint16_t>().max()) {
      throw std::runtime_error("File path too large: " +
                               file_info.relative_path);
    }

    uint16_t path_length =
        static_cast<uint16_t>(file_info.relative_path.size());

    utils::serialize(path_length, transfer_request_buffer);
    utils::serialize(file_info.relative_path, transfer_request_buffer);
    utils::serialize(file_info.size, transfer_request_buffer);
  }

  return transfer_request_buffer;
}

std::vector<std::filesystem::path> TransferRequest::get_file_paths() {
  std::vector<std::filesystem::path> file_paths;
  std::filesystem::path cwd = std::filesystem::current_path();

  for (const auto &file_info : file_infos_) {
    file_paths.emplace_back(cwd / file_info.relative_path);
  }

  return file_paths;
}

uint32_t TransferRequest::get_chunk_size() const {
  return uncompressed_chunk_size_;
}

void TransferRequest::print() const {
  std::cout << "Transfer Request Received\n";

  // Debug info
  std::cout << "Number of files: " << num_files_ << "\n";
  std::cout << "Total transfer size: " << transfer_size_ << " bytes\n";
  std::cout << "Uncompressed chunk size: " << uncompressed_chunk_size_
            << " bytes\n";
  std::cout << "Uncompressed last chunk size: "
            << uncompressed_final_chunk_size_ << " bytes\n";
  std::cout << "Number of chunks: " << num_chunks_ << "\n";

  for (const auto &file_info : file_infos_) {
    std::cout << "\t" << file_info.relative_path << " ("
              << utils::format_data_size(file_info.size) << ")" << std::endl;
  }

  std::cout << "Total " << num_files_ << " files ("
            << utils::format_data_size(transfer_size_) << ")" << std::endl;
}

const std::vector<TransferRequest::FileInfo> &
TransferRequest::get_file_infos() const {
  return file_infos_;
}

uint32_t TransferRequest::get_final_chunk_size() const {
  return uncompressed_final_chunk_size_;
}

uint32_t TransferRequest::get_num_chunks() const { return num_chunks_; }