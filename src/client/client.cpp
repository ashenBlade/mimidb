#include <algorithm>
#include <cctype>
#include <locale>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <iostream>
#include <vector>

#include "libmimi/MimiClient.hpp"

void sendTcl(mi::interface::libmimi::MimiClient &client, char command) {
    client.SendInt8(command);

    command = client.ReceiveInt8();

    if (command == 'O') {
        // Success
        return;
    }

    if (command == 'S') {
        auto str = client.ReceiveString();
        std::cout << str << std::endl;
        return;
    }
    
    throw std::runtime_error("unknown result");
}

static std::vector<int> recvTupleDescriptor(mi::interface::libmimi::MimiClient &client) {
    auto byte = client.ReceiveInt8();
    if (byte == 'S') {
        auto err = client.ReceiveString();
        std::cerr << "error: " << err << std::endl;
        throw std::runtime_error("");
    }

    if (byte != 'D') {
        std::cerr << "unknown byte " << byte << std::endl;
        throw std::runtime_error("");
    }

    auto natts = client.ReceiveInt32();
    auto lengths = std::vector<int>{};
    for (auto i = 0; i < natts; i++) {
        lengths.push_back(client.ReceiveInt32());
    }

    return lengths;
}

void handleTuple(mi::interface::libmimi::MimiClient &client, const std::vector<int> &lengths) {
    std::vector<std::string> attrs{lengths.size()};
    for (const auto length : lengths) {
        auto byte = client.ReceiveInt8();
        if (byte == '0') {
            attrs.push_back("NULL");
            continue;
        }
        assert(byte == '1');

        std::string val;
        switch (length) {
        case 8:
            val = std::to_string(client.ReceiveInt64());
            break;
        case 4:
            val = std::to_string(client.ReceiveInt32());
            break;
        case 2:
            val = std::to_string(client.ReceiveInt16());
            break;
        case 1:
            val = std::to_string(client.ReceiveInt8());
            break;
        case -1:
            val = client.ReceiveString();
            break;
        default:
            throw std::runtime_error("invalid length");
        }
        attrs.push_back(val);
    }

    for (const auto &attr : attrs) {
        std::cout << attr << " ";
    }

    std::cout << std::endl;
}

void sendSelect(mi::interface::libmimi::MimiClient &client) {
    client.SendInt8('S');

    auto attrs = recvTupleDescriptor(client);

    auto stop = false;
    while (!stop) {
        auto byte = client.ReceiveInt8();

        switch (byte) {
        case 'T':
            handleTuple(client, attrs);
            break;
        case 'E':
            stop = true;
            break;
        case 'S': {
            auto s = client.ReceiveString();
            std::cout << "error: " << s << std::endl;
            stop = true;
        } break;
        default:
            stop = true;
            std::cout << "unknown command " << byte << std::endl;
            break;
        }
    }
}

void sendInsert(mi::interface::libmimi::MimiClient &client) {
    // Пока только 2 числа есть в кортеже
    int32_t first;
    int16_t second;

    std::cin >> first >> second;

    client.SendInt8('I');
    client.SendInt32(first);
    client.SendInt16(second);

    auto ret = client.ReceiveInt8();
    if (ret == 'K') {
        // OK
    } else if (ret == 'S') {
        auto str = client.ReceiveString();
        std::cerr << "ERROR: " << str << std::endl;
    } else {
        std::cerr << "Unknown result byte " << ret << std::endl;
    }
}

int main() {
    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        std::cerr << "could not socket: " << strerror(errno) << std::endl;
        return 1;
    }

    auto addr = sockaddr_in{.sin_family = AF_INET,
                            .sin_port = htons(6543),
                            .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)},
                            .sin_zero = {}};

    if (connect(sock, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "could not connect: " << strerror(errno) << std::endl;
        return -1;
    }
    
    auto client = mi::interface::libmimi::MimiClient{sock};

    while (true) {
        std::string input;

        std::cout << "=> " << std::flush;
        std::cin >> input;

        if (!input.size()) {
            std::cerr << "ending" << std::endl;
            break;
        }

        // To lower case
        std::transform(input.begin(), input.end(), input.begin(),
                       [](char c) { return std::tolower(c, std::locale()); });

        try {
            if (input == "begin") {
                sendTcl(client, 'B');
            } else if (input == "commit") {
                sendTcl(client, 'C');
            } else if (input == "rollback") {
                sendTcl(client, 'R');
            } else if (input == "select") {
                sendSelect(client);
            } else if (input == "insert") {
                sendInsert(client);
            } else {
                std::cerr << "unknown command" << std::endl;
            }
        } catch (std::runtime_error &ex) {
            // ...
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    return 0;
}