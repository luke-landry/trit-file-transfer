#include <cstddef>
#include <vector>

class FileChunk {
    public:
        FileChunk(std::size_t size);

        char* data();
        const char* data() const;

        std::size_t size() const;

    private:
        std::vector<char> data_;
};