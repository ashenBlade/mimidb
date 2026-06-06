#include "ClientOptions.hpp"
#include "CommandSource.hpp"
#include "MimiClient.hpp"
#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/PacketType.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/detail/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cassert>
#include <cctype>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace mi::interface::libmimi;

// Global variable with options
static mi::client::ClientOptions Options;

static bool handleResponse(MimiClient &client) {
    TupleDescriptionPacket *tupleDescPacket = nullptr;
    while (auto response = client.ReceivePacket()) {
        auto stop = false;
        switch (response->Type()) {
        case PacketType::TupleDescription: {
            TupleDescriptionPacket *descr = dynamic_cast<TupleDescriptionPacket *>(response.get());
            for (const auto &att : descr->Attributes()) {
                std::cout << att.Name() << "\t";
            }
            std::cout << std::endl;
            tupleDescPacket = descr;
            break;
        }
        case PacketType::DataRow: {
            DataRowPacket *drow = dynamic_cast<DataRowPacket *>(response.get());
            if (!tupleDescPacket) {
                throw std::runtime_error("DataRow received before TupleDescription");
            }

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
            CommandCompletePacket *complete = dynamic_cast<CommandCompletePacket *>(response.get());

            std::cout << complete->GetTag();
            if (complete->GetRowsCount() >= 0) {
                std::cout << " " << complete->GetRowsCount();
            }
            std::cout << std::endl;
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

static bool tryConnectClient(int sock) {
    addrinfo hints{};
    hints.ai_flags = AI_CANONNAME | AI_NUMERICSERV;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo *result = nullptr;

    auto host = Options.Host.has_value() ? Options.Host.value() : "localhost";
    auto port = std::to_string(Options.Port.value_or(6543));

    auto ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (ret != 0) {
        std::cout << "could not get address info: " << gai_strerror(ret) << std::endl;
        return false;
    }

    // try connect to first address
    if ((ret = connect(sock, result->ai_addr, result->ai_addrlen)) != 0) {
        std::cout << "could not connect to database: " << strerror(errno) << std::endl;
        return false;
    }

    freeaddrinfo(result);
    return true;
}

bool tryCreateCommandSource(std::unique_ptr<mi::client::ICommandSource> &client) {
    if (Options.SingleCommand) {
        client =
            std::make_unique<mi::client::SingleCommandCommandSource>(Options.SingleCommand.value());
        return true;
    }

    int fd;
    bool interactive;
    if (Options.ScriptFile) {
        fd = open(Options.ScriptFile.value().c_str(), O_RDONLY);
        if (fd < 0) {
            std::cout << "could not open file: " << strerror(errno) << std::endl;
            return false;
        }

        interactive = false;
    } else {
        fd = STDIN_FILENO;
        interactive = true;
    }

    client = std::make_unique<mi::client::FDCommandSource>(fd, interactive);
    return true;
}

int main(int argc, const char **argv) {
    try {
        Options = mi::client::ClientOptions::ParseOptions(argc, argv);
    } catch (std::exception &ex) {
        std::cout << "could not parse options: " << ex.what() << std::endl;
        return 1;
    }

    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        std::cerr << "could not socket: " << strerror(errno) << std::endl;
        return 1;
    }

    if (!tryConnectClient(sock)) {
        return 1;
    }

    auto client = MimiClient{sock};
    std::unique_ptr<mi::client::ICommandSource> cmdsrc;
    if (!tryCreateCommandSource(cmdsrc)) {
        return 1;
    }

    while (true) {
        if (cmdsrc->IsInteractive()) {
            std::cout << "=> " << std::flush;
        }

        auto line = cmdsrc->GetNextLine();
        if (!line || line->size() == 0) {
            break;
        }

        auto &cmd = line.value();

        // Process special commands for interactive client
        if (cmdsrc->IsInteractive()) {
            if (cmd == "\\q") {
                break;
            }
        }

        auto packet = QueryPacket{std::move(cmd)};
        client.SendPacket(packet);

        try {
            if (!handleResponse(client)) {
                std::cout << "CONNECTION LOST" << std::endl;
                break;
            }
        } catch (std::exception &ex) {
            std::cout << "ERROR: " << ex.what() << std::endl;
            break;
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);
    return 0;
}