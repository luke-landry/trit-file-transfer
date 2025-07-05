#ifndef RECEIVER_H
#define RECEIVER_H

#include <string>
#include <atomic>
#include <optional>
#include <asio.hpp>

#include "TransferRequest.h"

#include "crypto.h"

class Receiver {
    public:
        Receiver(uint16_t port, const std::string& password);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::socket socket_;
        asio::ip::tcp::acceptor acceptor_;
        const std::string password_;

        std::string get_private_ipv4_address();
        void start_listening_for_connection();
        void wait_for_connection();
        bool receive_handshake();
        TransferRequest receive_transfer_request();
        bool accept_transfer_request(const TransferRequest& transfer_request);
        void receive_files(const TransferRequest& transfer_request);
};

#endif