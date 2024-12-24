
#include <random>
#include <asio.hpp>

#include "utils.h"

bool utils::is_valid_ip_address(const std::string& ip_str){
    std::error_code ec;

    // Using asio built-in checking for validating IP address string
    asio::ip::make_address(ip_str, ec);

    return !ec;
}

bool utils::is_valid_port(const std::string& port_str){

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

// Randomly generates a non-reserved port number
unsigned short utils::generate_random_port(){
    std::random_device rd;
    std::mt19937 gen(rd());

    // Port number must be 49152-65535 (non-reserved port range)
    std::uniform_int_distribution<> distribution(49152, 65535);

    return distribution(gen);
}

// Retuns a vector of strings delimited by whitespace from the given string
std::vector<std::string> utils::string_split(const std::string& str){
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
std::string utils::input<std::string>(const std::string& prompt, const std::vector<std::string>& expected_values){
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