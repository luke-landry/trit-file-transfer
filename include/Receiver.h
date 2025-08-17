#ifndef RECEIVER_H
#define RECEIVER_H

#include <string>
#include <optional>

#include "TcpSocket.h"
#include "TransferRequest.h"

#include "crypto.h"

class Receiver {
    public:
        Receiver(const std::string& ip, uint16_t port, const std::string& password);
        void start_session();

    private:
        const std::string ip_;
        const uint16_t port_;
        const std::string password_;
        TcpSocket sender_socket_;

        void start_listening_for_connection();
        void wait_for_connection();
        bool receive_handshake(std::optional<crypto::Decryptor>& decryptor_opt);
        TransferRequest receive_transfer_request();
        bool accept_transfer_request(const TransferRequest& transfer_request);
        void receive_files(const TransferRequest& transfer_request, crypto::Decryptor decryptor);
};

#endif