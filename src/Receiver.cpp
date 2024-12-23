#include <iostream>
#include <thread>
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
    acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), // Acceptor listening on all IPv4 interfaces (0.0.0.0)
    socket_(io_context_) {};

void Receiver::start_session(){

    std::string address = get_private_ipv4_address();
    const int port = acceptor_.local_endpoint().port();

    start_listening_for_connection();
    std::cout << "Listening for connection at address " << address << " on port " << port << std::endl;


    // Run io context for listening to connections in separate thread so that
    // user input can be handled in main thread
    std::thread io_thread([this]() {io_context_.run();});

    // Waits for user to press enter to stop
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    // Stop thread 
    io_context_.stop();
    if(io_thread.joinable()){
        io_thread.join();
    }
}

#ifdef __linux__
// Queries and returns the private ip address of this device to be used by the sender
// The first valid address (ipv4, not loopback, up, running) will be returned 
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

        std::cout << "Found network interface " << ifa->ifa_name << std::endl;
        
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

    // No valid address were found
    freeifaddrs(ifaddr);
    throw std::runtime_error("No valid network interfaces were found");
}
#endif

#ifdef _WIN32
// Queries and returns the private ipv4 address of this device to be used by the sender
std::string Receiver::get_private_ipv4_address(){
    // TODO Windows specific ip retrieval
}
#endif

void Receiver::start_listening_for_connection(){
    acceptor_.async_accept(socket_, [this](const asio::error_code& ec){
        const std::string sender_address_str = socket_.remote_endpoint().address().to_string();
        if(!ec){
            char choice = utils::input<char>("Accept connection from " + sender_address_str + "? (y/n)", {'y', 'n'});
            if(choice == 'y'){
                accept_connection();
            } else {
                deny_connection();
            }
        }

    });
}

void Receiver::accept_connection(){
    std::cout << "Connection accepted." << std::endl;
}

void Receiver::deny_connection(){
    socket_.close();
    std::cout << "Connection denied. Waiting for the next connection..." << std::endl;
    start_listening_for_connection();
}