
#include "TcpSocket.h"

TcpSocket::TcpSocket() : socket_(io_context_) {}

TcpSocket::~TcpSocket() { socket_.close(); }

void TcpSocket::connect(const std::string &ip, uint16_t port) {
  endpoint_ = asio::ip::tcp::endpoint(asio::ip::make_address(ip), port);
  socket_.connect(endpoint_);
}

void TcpSocket::accept(uint16_t port, TcpSocket &client_socket) {
  asio::ip::tcp::acceptor acceptor(
      client_socket.io_context_,
      asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
  acceptor.accept(client_socket.socket_);
  acceptor.close();
}

void TcpSocket::read(void *buf, std::size_t len) {
  asio::read(socket_, asio::buffer(buf, len));
}

void TcpSocket::write(const void *buf, std::size_t len) {
  asio::write(socket_, asio::buffer(buf, len));
}

void TcpSocket::close() {
  if (socket_.is_open()) {
    socket_.close();
  }
}

bool TcpSocket::is_open() const { return socket_.is_open(); }

std::string TcpSocket::remote_endpoint_address() const {
  return socket_.remote_endpoint().address().to_string();
}

uint16_t TcpSocket::remote_endpoint_port() const {
  return socket_.remote_endpoint().port();
}