#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

#include "Receiver.h"
#include "utils.h"

#ifdef __linux__
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
// TODO Windows specific includes
#endif

using asio::ip::tcp;

Receiver::Receiver(const unsigned short port):
    io_context_(),
    socket_(io_context_),
    acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    is_connected_(false) {};

void Receiver::start_session(){

    // Waits for a connection to be established (blocking)
    // Using blocking acceptor.accept() because the main thread has no 
    // concurrent tasks to perform while waiting for a connection
    wait_for_connection();

    wait_for_transfer_request();
}

#ifdef __linux__
// Queries and returns the private ip address of this device to be used by the sender
// The first valid address found (ipv4, not loopback, up, running) will be returned 
std::string Receiver::get_private_ipv4_address(){

    // Raw pointer to first element of linked list of network interface addresses returned by getifaddrs
    // Not using smart pointers because this struct uses a dedicated function to free its resources (freeifaddrs)
    struct ifaddrs* ifaddr = nullptr;

    if(getifaddrs(&ifaddr) == -1){
        throw std::runtime_error("Could not get network interfaces");
    }

    // Iterate through each interface address of interface addresses
    for(struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next){

        // Ignore interfaces without an address
        if(ifa->ifa_addr == nullptr){ continue; }
        
        // Read flags of network interface
        bool interface_address_is_ipv4 = ifa->ifa_addr->sa_family == AF_INET;
        bool interface_is_loopback = ifa->ifa_flags & IFF_LOOPBACK;
        bool interface_is_up = ifa->ifa_flags & IFF_UP;
        bool interface_is_running = ifa->ifa_flags & IFF_RUNNING;

        // If interface is valid, return its address as a string
        if(interface_address_is_ipv4 && !interface_is_loopback && interface_is_up && interface_is_running){

            // Reinterpret interface address specifically as ipv4 address (represented by sockaddr_in type)
            struct sockaddr_in* ipv4_address = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);

            // Buffer for raw ip address char string
            char address_chstr_buffer[INET_ADDRSTRLEN];

            // Convert ipv4 address to char string
            // where the address binary value buffer is stored in ipv4_address's struct member 'sin_addr'
            inet_ntop(AF_INET, &(ipv4_address->sin_addr), address_chstr_buffer, INET_ADDRSTRLEN);

            freeifaddrs(ifaddr);

            return std::string(address_chstr_buffer);
        }
    }

    // No valid ip address was found
    freeifaddrs(ifaddr);
    throw std::runtime_error("No valid IP address found");
}
#endif

#ifdef _WIN32
// Queries and returns the private ipv4 address of this device to be used by the sender
std::string Receiver::get_private_ipv4_address(){
    // TODO Windows specific ip retrieval
}
#endif

// Waits for a successful connection to be established
void Receiver::wait_for_connection(){
    std::string address = get_private_ipv4_address();
    const int port = acceptor_.local_endpoint().port();
    std::cout << "Listening for connection at address " << address << " on port " << port << "..." << std::endl;

    // Block until a connection is established
    // This will throw std::system_error if a connection fails
    acceptor_.accept(socket_);
    
    std::string sender_address = socket_.remote_endpoint().address().to_string();
    std::cout << "Connected to " << sender_address << std::endl;
}

void Receiver::wait_for_transfer_request(){
    std::cout << "Awaiting file transfer request..." << std::endl;
}