#ifndef SENDER_H
#define SENDER_H

#include "TcpSocket.h"
#include "crypto.h"
#include <string>

#include "TransferRequest.h"

class Sender {
public:
  Sender(const std::string &receiver_ip, const uint16_t receiver_port,
         const crypto::Key &key, const crypto::Salt &salt);
  void start_session();

private:
  const std::string receiver_ip_;
  const uint16_t receiver_port_;
  const crypto::Key key_;
  const crypto::Salt salt_;
  TcpSocket receiver_socket_;

  void connect_to_receiver();
  bool send_handshake(const crypto::Encryptor &encryptor);
  TransferRequest create_transfer_request();
  bool send_transfer_request(const TransferRequest &transfer_request);
  void send_files(const TransferRequest &transfer_request,
                  crypto::Encryptor encryptor);
};

#endif