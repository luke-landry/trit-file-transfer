
#include <vector>
#include <filesystem>

#include "BoundedThreadSafeQueue.h"
#include "Chunk.h"
#include "utils.h"

class FileChunker {
    public:
        FileChunker(const std::vector<std::filesystem::path>& file_paths, uint32_t chunk_size);
        void start(BoundedThreadSafeQueue<Chunk>& out_queue);

    private:
        std::vector<std::filesystem::path> file_paths_;
        const uint64_t chunk_size_;


};