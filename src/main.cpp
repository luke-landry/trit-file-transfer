
#include <iostream>
#include <vector>
#include <string>

#include "Sender.h"
#include "Receiver.h"
#include "utils.h"
#include "staging.h"
#include "crypto.h"

void handle_add(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        std::cerr << "trit: 'add' requires at least one file pattern.\n";
        std::cout << "usage: trit add <file_pattern>...\n";
        exit(1);
    }

    LOG("adding files to staging");
    staging::stage(args);
}

void handle_drop(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        std::cerr << "trit: 'drop' requires at least one file pattern.\n";
        std::cout << "usage: trit drop <file_pattern>...\n";
        exit(1);
    }

    LOG("dropping files from staging");
    staging::unstage(args);
}

void handle_list(const std::vector<std::string>& args) {
    LOG("listing staged files");
    staging::list();
}

void handle_clear(const std::vector<std::string>& args) {
    LOG("clearing staged files");
    staging::clear();
}

void handle_help(const std::vector<std::string>& args) {
    LOG("displaying help message");
    staging::help();
}

void handle_send(const std::vector<std::string>& args) {
    LOG("handling send command");

    if (args.size() != 2 && args.size() != 3) {
        std::cerr << "trit: 'send' requires an ip, port, and optionally a password.\n";
        std::cout << "usage: trit send <ip> <port> [password]\n";
        exit(1);
    }

    const std::string& ip = args[0];
    if (!utils::is_valid_ip_address(ip)) {
        std::cerr << "trit: invalid IP address\n";
        std::cout << "example: 192.168.1.10\n";
        exit(1);
    }

    const std::string& port_str = args[1];
    if (!utils::is_valid_port(port_str)) {
        std::cerr << "trit: invalid port\n";
        std::cout << "port must be a number between 1 and 65535\n";
        exit(1);
    }
    uint16_t port = std::stoi(port_str);

    const std::string& password = args.size() == 3 ? args[2] : "";

    crypto::init_sodium();
    LOG("initialized sodium");

    crypto::Salt salt;
    crypto::Key key(password, salt);

    // LOG("password=" + password);
    // LOG("salt=" + utils::buffer_to_hex_string(salt.data(), salt.size()));
    // LOG("key=" + utils::buffer_to_hex_string(key.data(), key.size()));

    LOG(std::string("sending to ") + ip + ":" + port_str);
    Sender sender(ip, port, key, salt);
    sender.start_session();
}

void handle_receive(const std::vector<std::string>& args) {
    LOG("handling receive command");

    /*
        Tried to pick a port number that was unreserved, unlikely to be used by other applications, and easy to type.
        Found an interesting website to search port information: https://www.speedguide.net/ports.php

        Was going to go with 50000, but this is used by many applications
        Then looked at 500001, but it turns out that Discord uses ports 50001-50004
        Tried 54321 but this is used by some software and unfortunately a bunch of malware
        Also tried 55555 but this one is associated with some malware as well
        Looked at 55000 but this is sometimes used by uTorrent and some other things as well
        Then looked at 50505 and 51515 but these are used by some applications
        Finally looked at 52525 and it had no documented existing users, so I picked this number
    */
    constexpr uint16_t DEFAULT_RECEIVER_PORT = 52525;

    if (args.size() > 1) {
        std::cerr << "trit: 'receive' only optionally takes a password.\n";
        std::cout << "usage: trit receive [password]\n";
        exit(1);
    }
    
    uint16_t port = DEFAULT_RECEIVER_PORT;
    while(!utils::local_port_available(port)){
        // Using randomly generated unreserved port for receiver if default is in use
        port = utils::generate_random_port();
    }

    const std::string& password = args.size() == 1 ? args[0] : "";
    // LOG("password=" + password);

    crypto::init_sodium();
    LOG("initialized sodium");

    LOG(std::string("receiving on port ") + std::to_string(port));
    Receiver receiver(port, password);
    receiver.start_session();
}

int main(int argc, char* argv[]){
    LOG("trit started");

    if(argc == 1){
        std::cerr << "trit: no command passed" << std::endl;
        std::cout << "See 'trit help' for a list of commands.\n";
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);

    const std::string& command = args[0];
    std::vector<std::string> command_args(args.begin() + 1, args.end());

    LOG("command: " + command);
    LOG("args: " + utils::str_join(command_args, ", "));

    using HandlerFunction = std::function<void(const std::vector<std::string>&)>;
    const std::unordered_map<std::string, HandlerFunction> command_dispatch_map = {
        {"add", handle_add},
        {"drop", handle_drop},
        {"list", handle_list},
        {"clear", handle_clear},
        {"help", handle_help},
        {"send", handle_send},
        {"receive", handle_receive}
    };

    auto it = command_dispatch_map.find(command);
    if (it != command_dispatch_map.end()) {
        it->second(command_args);
    } else {
        std::cerr << "trit: unknown command '" << command << "'\n";
        std::cout << "See 'trit help' for a list of commands.\n";
        return 1;
    }

    LOG("trit done");
    return 0;
}