#ifndef RECEIVER_H
#define RECEIVER_H

#include <string>
#include <atomic>
#include <asio.hpp>

class Receiver {
    public:
        Receiver(const unsigned short port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::socket socket_;
        asio::ip::tcp::acceptor acceptor_;
        std::atomic<bool> is_connected_;

        std::string get_private_ipv4_address();
        void start_listening_for_connection();
        void wait_for_connection();
        void wait_for_transfer_request();
};

#endif