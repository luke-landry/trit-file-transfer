
#include <random>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <asio.hpp>
#include <mutex>
#include <filesystem>
#include <fstream>

#include "utils.h"

namespace utils {

bool is_valid_ip_address(const std::string& ip_str){
    std::error_code ec;

    // Using asio built-in checking for validating IP address string
    asio::ip::make_address(ip_str, ec);

    return !ec;
}

bool is_valid_port(const std::string& port_str){

    if(port_str.empty()){
        return false;
    }

    if(!std::all_of(port_str.begin(), port_str.end(), ::isdigit)){
        return false;
    }

    unsigned long port_ul;

    // Catch if stoul throws error from value being too large for unsigned long
    try{
        port_ul = std::stoul(port_str);
    } catch (const std::exception&){
        return false;
    }

    
    return (port_ul >= 49152) && (port_ul <= 65535);
}

// Trick to get local IP address by creating a UDP socket and connecting to a remote endpoint (Google DNS)
std::optional<std::string> get_local_ipv4_address() {
    try {
        asio::io_context io;
        asio::ip::udp::socket sock(io);
        asio::ip::udp::endpoint dst(asio::ip::make_address_v4("8.8.8.8"), 53);
        sock.connect(dst);
        auto addr = sock.local_endpoint().address();
        if (addr.is_v4()) return addr.to_string();
        return std::nullopt; // could happen if system has no IPv4
    } catch (const std::exception&) {
        return std::nullopt; // no route or IPv4 found
    }
}

// Randomly generates a non-reserved port number
uint16_t generate_random_port(){
    std::random_device rd;
    std::mt19937 gen(rd());

    // Port number must be 49152-65535 (non-reserved port range)
    std::uniform_int_distribution<> distribution(49152, 65535);

    return distribution(gen);
}

bool local_port_available(uint16_t port){
    asio::io_context io_context;
    asio::error_code ec;
    asio::ip::tcp::acceptor acceptor(io_context);
    acceptor.open(asio::ip::tcp::v4(), ec);
    if(ec){ return false; }
    acceptor.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), ec);
    if(ec){ return false; }
    return true;
}

// Retuns a vector of strings delimited by whitespace from the given string
std::vector<std::string> string_split(const std::string& str){
    std::istringstream iss(str);
    std::vector<std::string> words;
    std::string word;

    while(iss >> word){
        words.push_back(word);
    }

    return words;
}

// Specializing input template function for string where getline is used 
// instead of cin so that entire string line is captured
template <>
std::string input<std::string>(const std::string& prompt, const std::vector<std::string>& expected_values){
    bool input_valid = false;
    std::string input;

    while(!input_valid){
        std::cout << prompt << std::endl;

        // Read entire line from cin
        std::getline(std::cin, input);

        // Check against allowed values (if any)
        if((!expected_values.empty()) && (std::find(expected_values.begin(), expected_values.end(), input) == expected_values.end())){
            std::cout << "Please enter an allowed option" << std::endl;
            continue;
        }

        input_valid = true;
    }

    return input;
}

// Specialization of serialization function template for std::string
template <>
void serialize<std::string>(const std::string& str, std::vector<uint8_t>& buffer){
    // Insert the chars of the string into the byte vector buffer
    buffer.insert(buffer.end(), str.begin(), str.end());
}

std::string buffer_to_hex_string(const uint8_t* buffer, const size_t size) {
    std::ostringstream oss;

    // Set hex output format with 2 digits, zero-filled
    oss << std::hex << std::setfill('0') << std::uppercase;

    for(size_t i = 0; i < size; ++i){

        // Add newline every before every 8th byte (except 0th byte)
        if ((i != 0) && (i % 8 == 0)) {
            oss << '\n';
        }

        oss << std::setw(2) << static_cast<int>(buffer[i]);

        // Add space only if not end of line or end of buffer
        if ((i + 1) % 8 != 0 && (i + 1) != size) {
            oss << ' ';
        }
    }

    if (size != 0) {
        oss << '\n';
    }

    return oss.str();
}

// Specialization of deserialization function template for std::string
// Caller must resize out_value string to their desired length before calling this function
template<>
std::vector<uint8_t>::const_iterator deserialize<std::string>(const std::vector<uint8_t>::const_iterator it, const std::vector<uint8_t>::const_iterator end, std::string& out_value){

    size_t string_size = out_value.size();
    auto buffer_distance = end - it;

    if (buffer_distance < 0) { throw std::runtime_error("Invalid iterator range (end before it)"); }
    if(buffer_distance < string_size){ throw std::runtime_error("Invalid buffer size"); }

    // Copy bytes of buffer into string
    std::copy(it, it + string_size, out_value.begin());

    return it + string_size;
}

std::string format_data_size(uint64_t size_in_bytes){

    if(size_in_bytes == 0){
        return "0 B";
    }

    // Cast from double to int, truncating decimals
    int power_of_1000 = static_cast<int>(std::log10(size_in_bytes) / 3.0);

    if(power_of_1000 == 0){
        return std::to_string(size_in_bytes) + " B";
    }

    if(power_of_1000 > 5){
        throw std::runtime_error("Data size too large to format");
    }

    // First element is a dummy element to align powers of 1000 indices with corresponding units
    std::vector<std::string> units = {"", " KB", " MB", " GB", " TB", " PB"};

    // Unit value after converting unit to a power of 1000 (e.g 10524 -> 10.524)
    double unit_value = size_in_bytes / (std::pow(1000, power_of_1000));
    
    // Format unit value to two decimal places (e.g. 10.524 -> 10.52)
    std::ostringstream unit_value_oss;
    unit_value_oss << std::fixed << std::setprecision(2) << unit_value;

    // Return <unit value> <units>
    return unit_value_oss.str() + units[power_of_1000];
}

std::string get_timestamp(const std::string& format){
    time_t  current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&current_time), format.c_str());
    return timestamp.str();
}

void log(const std::string& message) {
    static const std::filesystem::path log_path = 
        std::filesystem::temp_directory_path() / "trit" / ("log_" + std::to_string(getpid()) + ".txt");
    static std::ofstream ofs;
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    if(!ofs.is_open()){
        std::filesystem::create_directories(log_path.parent_path());
        ofs.open(log_path, std::ios::trunc);
        if(!ofs){
            std::cerr << "Failed to open log file at " << log_path << "\n";
            return;
        }
    }

    ofs << "[" << get_timestamp("%T") << "] " << message << std::endl;
}

std::filesystem::path relative_to_cwd(const std::filesystem::path& path){
    static auto cwd = std::filesystem::current_path();
    return std::filesystem::relative(path, cwd);
}

std::unordered_set<std::filesystem::path> relative_to_cwd(const std::unordered_set<std::filesystem::path>& paths){
    std::unordered_set<std::filesystem::path> rel_paths;
    rel_paths.reserve(paths.size());
    for(const auto& path: paths){
        rel_paths.insert(relative_to_cwd(path));
    }
    return rel_paths;
}

std::string str_join(const std::vector<std::string>& strings, const std::string& delimiter){
    std::ostringstream oss;
    for (auto it = strings.begin(); it != strings.end(); ++it) {
        oss << *it;
        if (std::next(it) != strings.end()) {
            oss << delimiter;
        }
    }
    return oss.str();
}

} // namespace utils