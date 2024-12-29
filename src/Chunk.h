
#include <cstdint>
#include <vector>

class Chunk {
    public:
        Chunk(unsigned int sequence_num, std::size_t size);

        unsigned int sequence_num();
        const unsigned int sequence_num() const;

        uint8_t* data();
        const uint8_t* data() const;

        std::size_t size() const;

    private:
        unsigned int sequence_num_;
        std::vector<uint8_t> data_;
};