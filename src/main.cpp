
#include <iostream>
#include <vector>
#include <string>

#include "Sender.h"
#include "Receiver.h"
#include "utils.h"
#include "staging.h"

void handle_add(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "trit: 'add' requires at least one file pattern.\n";
        std::cout << "usage: trit add <file_pattern>...\n";
        exit(1);
    }
    std::vector<std::string> file_patterns(args.begin() + 1, args.end());

    LOG("adding files to staging");
    staging::stage(file_patterns);
}

void handle_drop(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "trit: 'drop' requires at least one file pattern.\n";
        std::cout << "usage: trit drop <file_pattern>...\n";
        exit(1);
    }
    std::vector<std::string> file_patterns(args.begin() + 1, args.end());

    LOG("dropping files from staging");
    staging::unstage(file_patterns);
}

void handle_list(const std::vector<std::string>&) {
    LOG("listing staged files");
    staging::list();
}

void handle_clear(const std::vector<std::string>&) {
    LOG("clearing staged files");
    staging::clear();
}

void handle_help(const std::vector<std::string>&) {
    LOG("displaying help message");
    staging::help();
}

void handle_send(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        std::cerr << "trit: 'send' requires an ip and port.\n";
        std::cout << "usage: trit send <ip> <port>\n";
        exit(1);
    }

    const std::string& ip = args[1];
    if (!utils::is_valid_ip_address(ip)) {
        std::cerr << "trit: invalid IP address\n";
        std::cout << "example: 192.168.1.10\n";
        exit(1);
    }

    const std::string& port_str = args[2];
    if (!utils::is_valid_port(port_str)) {
        std::cerr << "trit: invalid port\n";
        std::cout << "port must be a number between 1 and 65535\n";
        exit(1);
    }
    uint16_t port = std::stoi(port_str);

    LOG(std::string("sending to ") + ip + ":" + port_str);
    Sender sender(ip, port);
    sender.start_session();
}

void handle_receive(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        std::cerr << "trit: 'receive' takes no additional arguments.\n";
        std::cout << "usage: trit receive\n";
        exit(1);
    }

    // Using randomly generated unreserved port for receiver
    uint16_t port = utils::generate_random_port();
    while(!utils::local_port_available(port)){
        port = utils::generate_random_port();
    }

    LOG(std::string("receiving on port ") + std::to_string(port));
    Receiver receiver(port);
    receiver.start_session();
}

int main(int argc, char* argv[]){
    LOG("trit started");

    if(argc == 1){
        std::cerr << "trit: no arguments passed" << std::endl;
        std::cout << "See 'trit help' for a list of commands.\n";
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    const std::string& command = args[0];

    if (command == "add") {
        handle_add(args);
    } else if (command == "drop") {
        handle_drop(args);
    } else if (command == "list") {
        handle_list(args);
    } else if (command == "clear") {
        handle_clear(args);
    } else if (command == "help") {
        handle_help(args);
    } else if (command == "send") {
        handle_send(args);
    } else if (command == "receive") {
        handle_receive(args);
    } else {
        std::cerr << "trit: unknown command '" << command << "'\n";
        std::cout << "See 'trit help' for a list of commands.\n";
        return 1;
    }

    LOG("trit done");
    return 0;
}