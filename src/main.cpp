
#include <iostream>
#include <vector>
#include <string>

#include "utils.h"
#include "Sender.h"
#include "Receiver.h"

// Usage: 'trit send <ip> <port>' or 'trit receive'
int main(int argc, char* argv[]){

    if(argc == 1){
        std::cout << "Usage: 'trit send <ip> <port>' or 'trit receive'" << std::endl;
        return 0;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    const std::string& mode = args[0];
    if(mode == "send"){

        if(args.size() != 3){
            std::cout << "Send mode needs ip address and port: trit send <ip> <port>" << std::endl;
            return 0;
        }

        const std::string& ip_address_str = args[1];
        const std::string& port_str = args[2];

        if(!utils::is_valid_ip_address(ip_address_str)){
            std::cout << "Invalid IP address" << std::endl;
            return 0;
        }
        
        if (!utils::is_valid_port(port_str)){
            std::cout << "Invalid port" << std::endl;
            return 0;
        }

        int port = std::stoi(port_str);

        try{
            Sender sender(ip_address_str, port);
            sender.start_session();
        } catch(const std::system_error& e){
            std::cout << "Exception: " << e.what() << std::endl;

        } catch(const std::runtime_error& e){
            std::cout << "Exception: " << e.what() << std::endl;
        }

    } else if(mode == "receive"){

        // Using randomly generated unreserved port for receiver
        uint16_t port = utils::generate_random_port();
        while(!utils::local_port_available(port)){
            port = utils::generate_random_port();
        }

        if(args.size() != 1){
            std::cout << "Receive mode takes no parameters: trit receive" << std::endl;
            return 0;
        }

        try{
            Receiver receiver(port);
            receiver.start_session();
        } catch(const std::system_error& e){
            std::cout << "Exception: " << e.what() << std::endl;

        } catch(const std::runtime_error& e){
            std::cout << "Exception: " << e.what() << std::endl;
        }

    } else{
        std::cout << "Please enter a valid mode: either 'send' with two parameters or 'receive' with no parameters" << std::endl;
        std::cout << "Usage: 'trit send <ip> <port>' or 'trit receive'" << std::endl;
    }

    return 0;
}