#ifndef TRANSFER_REQUEST_H
#define TRANSFER_REQUEST_H

#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>


class TransferRequest {
    public:

        // Helper struct to store file path and size pairs to be used in file_infos vector
        // Not using a map for this because access is mostly sequential,
        // and sequential access is more efficent with vectors than with maps
        struct FileInfo {
            std::string relative_path;
            uint64_t size;
            FileInfo(const std::string& p, uint64_t s): relative_path(p), size(s) {}
        };

        static TransferRequest from_file_paths(const std::unordered_set<std::filesystem::path>& file_paths);
        static TransferRequest deserialize(const std::vector<uint8_t>& buffer);
        std::vector<uint8_t> serialize() const;
        std::vector<std::filesystem::path> get_file_paths();
        uint32_t get_chunk_size();

        void print() const;

    private:

        TransferRequest(uint32_t num_files, 
                        uint64_t transfer_size, 
                        uint32_t uncompressed_chunk_size, 
                        uint32_t uncompressed_last_chunk_size, 
                        uint32_t num_chunks,
                        std::vector<FileInfo> file_infos);
    
        uint32_t num_files_;
        uint64_t transfer_size_;
        uint32_t uncompressed_chunk_size_;
        uint32_t uncompressed_last_chunk_size_;
        uint32_t num_chunks_;
        std::vector<TransferRequest::FileInfo> file_infos_;
};

#endif