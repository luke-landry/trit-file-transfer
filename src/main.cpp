
#include <iostream>
#include <vector>
#include <string>

#include "utils.h"
#include "Sender.h"
#include "Receiver.h"

// Usage: 'trit send <ip> <port>' or 'trit receive'
int main(int argc, char* argv[]){

    if(argc == 1){
        std::cout << "Usage: 'trit send <ip> <port>' or 'trit receive <port/optional>'" << std::endl;
        return 0;
    }

    const std::vector<std::string> args(argv + 1, argv + argc);
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

        Sender sender;
        sender.start_session(ip_address_str, port);

    } else if(mode == "receive"){
        int port;

        if(args.size() == 1){

            // Using port 50505 as default port for this program
            port = 50505;            
            
        } else if(args.size() == 2) {
            const std::string& port_str = args[1];

            if (!utils::is_valid_port(port_str)){
                std::cout << "Invalid port" << std::endl;
                return 0;
            }

            port = std::stoi(port_str);

        } else {
            std::cout << "Receive mode takes maximum one parameter: trit receive <port/optional>" << std::endl;
            return 0;
        }

        Receiver receiver;
        receiver.start_session(port);

    } else{
        std::cout << "Please enter a valid mode: either 'send' with two parameters or 'receive' with no parameters" << std::endl;
        std::cout << "Usage: 'trit send <ip> <port>' or 'trit receive'" << std::endl;
    }

    return 0;
}