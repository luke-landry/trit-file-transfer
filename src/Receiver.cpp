#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

#include "Receiver.h"
#include "utils.h"
#include "TransferManager.h"
#include "FileManager.h"
#include "ProgressTracker.h"
#include "CompressionManager.h"

#ifdef __linux__
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

Receiver::Receiver(uint16_t port, const std::string& password):
    io_context_(),
    socket_(io_context_),
    acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    password_(password) {}

void Receiver::start_session(){
    LOG("receiver session started");

    wait_for_connection();
    LOG("connected to sender");

    if(!receive_handshake()){
        std::cout << "Handshake failed. Ensure passwords match." << std::endl;
        return;
    }
    LOG("handshake successful");

    TransferRequest transfer_request = receive_transfer_request();
    LOG("receieved transfer request");

    while(!accept_transfer_request(transfer_request)){
        LOG("transfer request denied by user");
        transfer_request = receive_transfer_request();
        LOG("receieved transfer request");
    }
    LOG("transfer request accepted by user");

    LOG("starting transfer receive");
    receive_files(transfer_request);
    LOG("transfer receieved and completed");
}

// Queries and returns the private ip address of this device to be used by the sender
// The first valid address found (ipv4, not loopback, up, running) will be returned 
std::string Receiver::get_private_ipv4_address(){
    struct ifaddrs* ifaddr = nullptr;
    if(getifaddrs(&ifaddr) == -1){
        throw std::runtime_error("Could not get network interfaces");
    }

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

// Waits for a successful connection to be established
void Receiver::wait_for_connection(){
    LOG("waiting for connection from sender");
    std::string address = get_private_ipv4_address();
    const uint16_t port = acceptor_.local_endpoint().port();
    std::cout << "Listening for connection at " << address << " on port " << port << "..." << std::endl;
    acceptor_.accept(socket_);
    std::string sender_address = socket_.remote_endpoint().address().to_string();
    std::cout << "Connected to " << sender_address << std::endl;
}

// Handshake format: salt, nonce, cipher
bool Receiver::receive_handshake(){
    LOG("receiving handshake");

    LOG("receiving salt");
    std::array<uint8_t, crypto::SALT_SIZE> salt_buffer;
    asio::read(socket_, asio::buffer(salt_buffer.data(), salt_buffer.size()));
    crypto::Salt salt(salt_buffer);

    // TODO remove this log
    LOG("salt=" + utils::buffer_to_hex_string(salt.data(), salt.size()));

    LOG("deriving key");
    crypto::Key key(password_, salt);

    // TODO remove this log
    LOG("key=" + utils::buffer_to_hex_string(key.data(), key.size()));

    LOG("receiving handshake nonce");
    std::array<uint8_t, crypto::NONCE_SIZE> nonce_buffer;
    asio::read(socket_, asio::buffer(nonce_buffer.data(), nonce_buffer.size()));
    crypto::Nonce nonce(nonce_buffer);
    LOG("nonce=" + utils::buffer_to_hex_string(nonce.data(), nonce.size()));

    LOG("receiving handshake cipher");
    std::array<uint8_t, crypto::HANDSHAKE_CIPHERTEXT_SIZE> ciphertext;
    asio::read(socket_, asio::buffer(ciphertext.data(), ciphertext.size()));
    LOG("ciphertext=" + utils::buffer_to_hex_string(ciphertext.data(), ciphertext.size()));

    bool handshake_success = crypto::verify_handshake_tag(key, nonce, ciphertext);
    uint8_t response_byte = handshake_success ? 1 : 0;
    asio::write(socket_, asio::buffer(&response_byte, sizeof(response_byte)));
    return handshake_success;
}

/*
    This function waits for, receives, and deserializes the file transfer request from the sender
    --------------------------
    Transfer request format:
    transfer reqeust size [8 bytes]

    number of files [4 bytes]
    total transfer size [8 bytes]
    uncompressed chunk size [4 bytes]
    uncompressed last chunk size [4 bytes]
    num chunks [4 bytes]
    file1 path length [2 bytes]
    file1 path [variable]
    file1 size [8 bytes]
    file2 path length [2 bytes]
    file2 path [variable]
    file2 size [8 bytes]
    ...
    fileN path length [2 bytes]
    fileN path [variable]
    fileN size [8 bytes]
*/
TransferRequest Receiver::receive_transfer_request(){
    std::cout << "Awaiting file transfer request..." << std::endl;
    uint64_t transfer_request_buffer_size;
    asio::read(socket_, asio::buffer(&transfer_request_buffer_size, sizeof(transfer_request_buffer_size)));
    std::vector<uint8_t> transfer_request_buffer(transfer_request_buffer_size);
    asio::read(socket_, asio::buffer(transfer_request_buffer));
    return TransferRequest::deserialize(transfer_request_buffer);
}

bool Receiver::accept_transfer_request(const TransferRequest& transfer_request){
    transfer_request.print();
    char choice = utils::input<char>("Accept transfer request? (y/n)", {'y', 'n'});
    bool request_accepted = choice == 'y';
    uint8_t request_accepted_byte = request_accepted;
    asio::write(socket_, asio::buffer(&request_accepted_byte, sizeof(request_accepted_byte)));
    std::cout << ((request_accepted ? "Transfer accepted" : "Transfer denied")) << std::endl;
    return request_accepted;
}

void Receiver::receive_files(const TransferRequest& transfer_request){
    std::cout << "Receiving files..." << std::endl;
    auto start_time = std::chrono::system_clock::now();

    uint32_t num_chunks = transfer_request.get_num_chunks();

    constexpr int QUEUE_CAPACITY = 50;
    BoundedThreadSafeQueue<std::unique_ptr<Chunk>> received_chunk_queue(QUEUE_CAPACITY);

    std::atomic<bool> chunk_reception_done(false);
    std::atomic<bool> decompression_done(false);
    std::atomic<uint32_t> chunks_written(0);

    TransferManager chunk_receiver;
    std::thread receiver_thread([&](){
        chunk_receiver.receive_chunks(
            socket_,
            received_chunk_queue,
            chunk_reception_done,
            transfer_request.get_num_chunks()
        );
    });

    FileManager file_writer;
    std::thread writer_thread([&](){
        file_writer.write_files_from_chunks(transfer_request,
            received_chunk_queue,
            chunk_reception_done,
            chunks_written
        );
    });

    ProgressTracker progress_tracker("Chunks written", num_chunks);
    std::thread progress_thread([&](){
        progress_tracker.start(chunks_written);
    });

    receiver_thread.join();
    writer_thread.join();
    progress_thread.join();

    auto end_time = std::chrono::system_clock::now();
    auto time_elapsed = std::chrono::duration<double>(end_time-start_time);
    auto seconds_elapsed = time_elapsed.count();
    
    std::cout << "Files received, transfer complete!" << std::endl;
    std::cout << "Time elapsed: " << seconds_elapsed << "s" << std::endl;
}