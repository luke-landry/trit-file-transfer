
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

    // Port number must be 49152-65535 (non-reserved port range)
    return (port_ul >= 49152) && (port_ul <= 65535);
}