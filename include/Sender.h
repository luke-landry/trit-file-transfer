#ifndef SENDER_H
#define SENDER_H

#include <asio.hpp>
#include <string>
#include "crypto.h"

#include "TransferRequest.h"

class Sender {
    public:
        Sender(const std::string& recipient_ip_address_str, const uint16_t recipient_port, const crypto::Key& key, const crypto::Salt& salt);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::endpoint receiver_endpoint_;
        asio::ip::tcp::socket socket_;
        const crypto::Key key_;
        const crypto::Salt salt_;

        void connect_to_receiver();
        bool send_handshake(const crypto::Encryptor& encryptor);
        TransferRequest create_transfer_request();
        bool send_transfer_request(const TransferRequest& transfer_request);
        void send_files(const TransferRequest& transfer_request, crypto::Encryptor encryptor);
       
};

#endif