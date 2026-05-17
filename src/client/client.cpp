#include "MimiClient.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/PacketType.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <locale>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace mi::interface::libmimi;

static bool handleResponse(MimiClient &client) {
    // XXX: тут можно создать стейт машину, чтобы проверять разные шняги, например, что мне всегда
    // приходит TupleDescr перед DataRow
    while (auto response = client.ReceivePacket()) {
        auto stop = false;
        switch (response->Type()) {
        case PacketType::TupleDescription: {
            TupleDescriptionPacket *descr = dynamic_cast<TupleDescriptionPacket *>(response.get());
            for (const auto &att : descr->Attributes()) {
                std::cout << att.Name() << "\t";
            }
            std::cout << std::endl;
            break;
        }
        case PacketType::DataRow: {
            DataRowPacket *drow = dynamic_cast<DataRowPacket *>(response.get());
            for (const auto &val : drow->Values()) {
                if (val.has_value()) {
                    std::cout << val.value() << "\t";
                } else {
                    std::cout << "NULL" << "\t";
                }
            }
            std::cout << std::endl;
            break;
        }
        case PacketType::CommandComplete: {
            stop = true;
            break;
        }
        case PacketType::ErrorResponse: {
            ErrorResponsePacket *err = dynamic_cast<ErrorResponsePacket *>(response.get());
            std::cerr << "ERROR: " << err->Message() << std::endl;
            stop = true;
            break;
        }
        case PacketType::Query: {
            std::cerr << "Must not receive QUERY packet at client" << std::endl;
            exit(1);
            break;
        }
        }

        if (stop) {
            // Success
            return true;
        }
    }

    // If we are here it means that connection lost
    return false;
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

    auto client = MimiClient{sock};

    while (true) {
        std::string input;

        std::cout << "=> " << std::flush;
        std::getline(std::cin, input);

        // To lower case
        std::transform(input.begin(), input.end(), input.begin(),
                       [](char c) { return std::tolower(c, std::locale()); });

        if (!input.size()) {
            // Just continue
            continue;
        }
        if (input == "\\q" || input == "q") {
            break;
        }

        auto packet = QueryPacket{std::move(input)};
        client.SendPacket(packet);

        if (!handleResponse(client)) {
            std::cout << "CONNECTION LOST" << std::endl;
            break;
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    return 0;
}