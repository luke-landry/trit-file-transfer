#ifndef SENDER_H
#define SENDER_H

#include <asio.hpp>
#include <string>

#include "StagingArea.h"
#include "TransferRequest.h"

class Sender {
    public:
        Sender(const std::string recipient_ip_address_str, const unsigned short recipient_port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::endpoint receiver_endpoint_;
        asio::ip::tcp::socket socket_;
        StagingArea stagingArea_;

        void connect_to_receiver();
        void stage_files_for_transfer();
        TransferRequest send_transfer_request();
        bool transfer_request_accepted();
        void send_files(const TransferRequest& transfer_request);
};

#endif