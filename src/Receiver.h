#ifndef RECEIVER_H
#define RECEIVER_H

#include <string>
#include <atomic>
#include <asio.hpp>

#include "TransferRequest.h"

class Receiver {
    public:
        Receiver(const unsigned short port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::socket socket_;
        asio::ip::tcp::acceptor acceptor_;

        std::string get_private_ipv4_address();
        void start_listening_for_connection();
        void wait_for_connection();
        TransferRequest receive_transfer_request();
        bool accept_transfer_request(const TransferRequest& transfer_request);
        void receive_files();
};

#endif