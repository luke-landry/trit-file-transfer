
#include <string>

class Sender {
    public:
        Sender(const std::string recipient_ip_address_str, const int recipient_port);
        void start_session();

    private:
        const std::string ip_address_str;
        const int port;

};