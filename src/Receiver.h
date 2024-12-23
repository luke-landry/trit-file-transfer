
#include <string>
#include <asio.hpp>

class Receiver {
    public:
        Receiver(const unsigned short port);
        void start_session();

    private:
        asio::io_context io_context_;
        asio::ip::tcp::acceptor acceptor_;
        asio::ip::tcp::socket socket_;

        std::string get_private_ipv4_address();
        void start_listening_for_connection();
        void accept_connection();
        void deny_connection();
};