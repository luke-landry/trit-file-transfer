#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <asio.hpp>
#include <chrono>
#include <cstddef>
#include <system_error>

// Simple blocking TCP socket wrapper
class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket();

    // non-copyable, non-movable
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&&) = delete;
    TcpSocket& operator=(TcpSocket&&) = delete;

    void connect(const std::string& ip, uint16_t port);
    static void accept(uint16_t port, TcpSocket& client_socket);
    void read(void* buf, std::size_t len);
    void write(const void* buf, std::size_t len);
    void close();
    bool is_open() const;

    std::string remote_endpoint_address() const;
    uint16_t remote_endpoint_port() const;

private:
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::endpoint endpoint_;
};

#endif // TCPSOCKET_H
