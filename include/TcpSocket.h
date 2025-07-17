#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class TcpSocket {
public:

    TcpSocket();
    TcpSocket(int fd);
    ~TcpSocket();

    // Sockets should not be copied, only moved
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;


    void connect_to(const std::string& ip, uint16_t port);
    void bind_and_listen(uint16_t port);
    TcpSocket accept_connection();

    void send_data(const void* data, size_t size);
    void receive_data(void* data, size_t size);

    // Returns remote IP address (after connected/accepted)
    std::string remote_ip() const;

    void close();

private:
    int sock_fd_;
    void assert_valid() const;
};

#endif // TCPSOCKET_H
