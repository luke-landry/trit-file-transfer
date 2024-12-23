#ifndef SENDER_H
#define SENDER_H

#include <atomic>
#include <asio.hpp>

#include <string>

class Sender {
    public:
        Sender(const std::string recipient_ip_address_str, const unsigned short recipient_port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::endpoint receiver_endpoint_;
        asio::ip::tcp::socket socket_;

        void connect_to_receiver();
        void stage_files_for_transfer();
        void send_transfer_request();
        void send_files();
};

#endif