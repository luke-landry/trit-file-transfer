#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <bit>
#include <limits>

// Using namespace instead of class to group utility functions since they are stateless and no data is shared between them
namespace utils {

    // Shared constants
    constexpr uint32_t MAX_UNCOMPRESSED_CHUNK_SIZE = 4096;

    // Non-template function declarations
    bool is_valid_ip_address(const std::string& ip);
    bool is_valid_port(const std::string& port);
    uint16_t generate_random_port();
    std::vector<std::string> string_split(const std::string& str);
    void print_buffer(std::vector<uint8_t> buffer);
    uint64_t deserialize_uint(std::vector<uint8_t> buffer, uint8_t size);
    std::string format_data_size(uint64_t size_in_bytes);
    std::string get_timestamp(const std::string& format);

    // Template function definitions

    /*
    Prompts for and validates user input and checks it against a vector of expected values.
    If a vector of expected values is not provided, then any valid input is allowed.
    */
    template <typename T>
    T input(const std::string& prompt = "", const std::vector<T>& expected_values = {}){
        bool input_valid = false;
        T input;

        while(!input_valid){
            std::cout << prompt << std::endl;
            std::cin >> input;

            // Check for input stream error (due to invalid input) and handle failure
            if(std::cin.fail()){
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input" << std::endl;
                continue;
            }

            // Check against allowed values (if any)
            if((!expected_values.empty()) && (std::find(expected_values.begin(), expected_values.end(), input) == expected_values.end())){
                std::cout << "Please enter an allowed option" << std::endl;
                continue;
            }

            input_valid = true;
        }

        return input;
    }

    // Specialization of input function template necessary for getting entire line of string
    template <>
    std::string input<std::string>(const std::string& prompt, const std::vector<std::string>& expected_values);

    // Serializes the given value and appends it to the byte vector buffer
    template <typename T>
    void serialize(const T& value, std::vector<uint8_t>& buffer){

        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable to be serialized properly");

        // Gets a char pointer to the first byte of the value's data in memory
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&value);
        const size_t data_size = sizeof(T);
        buffer.insert(buffer.end(), data, data + data_size);
    }

    // Specialization of serialization function template for std::string
    template <>
    void serialize<std::string>(const std::string& str, std::vector<uint8_t>& buffer);

    // Deserializes buffer data into out_value
    // assumes buffer byte order is the same as native byte order (should be little endian)
    template<typename T>
    std::vector<uint8_t>::const_iterator deserialize(const std::vector<uint8_t>::const_iterator it, const std::vector<uint8_t>::const_iterator end, T& out_value){

        // Only allow integral types (integer based types)
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable to be deserialized properly");

        size_t int_size = sizeof(T);
        auto buffer_distance = end - it;

        if(buffer_distance < int_size){ throw std::runtime_error("Invalid buffer size"); }

        // Vectors store elements contiguously, so its safe to use pointers to dereferenced iterator values for memcpy
        std::memcpy(&out_value, &(*it), int_size);

        return it + int_size;
    }

    template<>
    std::vector<uint8_t>::const_iterator deserialize<std::string>(const std::vector<uint8_t>::const_iterator it, const std::vector<uint8_t>::const_iterator end, std::string& out_value);

};

#endif