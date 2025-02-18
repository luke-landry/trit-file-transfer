#ifndef SENDER_H
#define SENDER_H

#include <asio.hpp>
#include <string>

#include "StagingArea.h"
#include "TransferRequest.h"

class Sender {
    public:
        Sender(const std::string recipient_ip_address_str, const uint16_t recipient_port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::endpoint receiver_endpoint_;
        asio::ip::tcp::socket socket_;
        StagingArea stagingArea_;

        void connect_to_receiver();
        TransferRequest stage_files_for_transfer();
        bool send_transfer_request(const TransferRequest& transfer_request);
        void send_files(const TransferRequest& transfer_request);
};

#endif