
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>


class TransferRequest {
    public:
        static TransferRequest from_file_paths(const std::unordered_set<std::filesystem::path>& file_paths);
        static TransferRequest deserialize(const std::vector<uint8_t>& buffer);
        std::vector<uint8_t> serialize();
        void print();

    private:

        // Helper struct to store file path and size pairs to be used in file_infos vector
        // Not using a map for this because access is mostly sequential,
        // and sequential access is more efficent with vectors than with maps
        struct FileInfo {
            std::string path;
            uint64_t size;
            FileInfo(const std::string& p, uint64_t s): path(p), size(s) {}
        };


        TransferRequest(uint32_t num_files, 
                        uint64_t transfer_size, 
                        uint32_t uncompressed_chunk_size, 
                        uint32_t uncompressed_last_chunk_size, 
                        uint32_t num_chunks,
                        std::vector<FileInfo> file_infos);
    
        const uint32_t num_files_;
        const uint64_t transfer_size_;
        const uint32_t uncompressed_chunk_size_;
        const uint32_t uncompressed_last_chunk_size_;
        const uint32_t num_chunks_;
        const std::vector<FileInfo> file_infos_;
};