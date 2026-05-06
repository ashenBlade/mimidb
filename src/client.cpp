#include <algorithm>
#include <cctype>
#include <locale>
#include <netinet/in.h>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <iostream>
#include <vector>

std::string receiveString(int sock) {
    int32_t length;
    if (recv(sock, &length, sizeof(int32_t), 0) < 0) {
        std::cerr << "could not recv: " << strerror(errno) << std::endl;
        return {};
    }

    // Пока знаю, что не больше 128 будет строка
    if (length < 0 || 128 < length) {
        std::cerr << "sus str len received: " << length << std::endl;
        return {};
    }

    auto left = length;
    auto result = std::string(static_cast<size_t>(length), 0);
    auto cursor = result.data();
    while (left > 0) {
        auto ret = recv(sock, cursor, static_cast<size_t>(left), 0);
        if (ret < 0) {
            std::cerr << "could not recv: " << strerror(errno) << std::endl;
            return {};
        }

        left -= static_cast<int32_t>(ret);
        cursor += ret;
    }

    return result;
}

void sendByte(int sock, char byte) {
    if (send(sock, &byte, sizeof(char), 0) < 0) {
        std::cerr << "could not send " << byte << ": " << strerror(errno) << std::endl;
        return;
    }
}

char recvByte(int sock) {
    char byte;
    if (recv(sock, &byte, sizeof(char), 0) < 0) {
        std::cerr << "could not recv: " << strerror(errno) << std::endl;
        throw std::runtime_error("");
    }

    return byte;
}

void sendTcl(int sock, char command) {
    sendByte(sock, command);

    command = recvByte(sock);

    if (command == 'O') {
        // Success
        return;
    }

    if (command == 'S') {
        auto str = receiveString(sock);
        std::cout << str << std::endl;
        return;
    }
}

static std::vector<int> recvTupleDescriptor(int sock) {
    char byte;
    if (recv(sock, &byte, sizeof(char), 0) < 0) {
        std::cerr << "could not recv " << strerror(errno) << std::endl;
        throw std::runtime_error("");
    }

    if (byte == 'S') {
        auto err = receiveString(sock);
        std::cerr << "error: " << err << std::endl;
        throw std::runtime_error("");
    }

    if (byte != 'D') {
        std::cerr << "unknown byte " << byte << std::endl;
        throw std::runtime_error("");
    }

    int32_t natts;
    if (recv(sock, &natts, sizeof(int32_t), 0) != sizeof(int32_t)) {
        std::cerr << "could not recv " << strerror(errno) << std::endl;
        throw std::runtime_error("");
    }

    auto lengths = std::vector<int>{};
    for (auto i = 0; i < natts; i++) {
        int32_t length;
        if (recv(sock, &length, sizeof(int32_t), 0) != sizeof(int32_t)) {
            std::cerr << "could not recv " << strerror(errno) << std::endl;
            throw std::runtime_error("");
        }

        lengths.push_back(length);
    }

    return lengths;
}

template <class T> T readScalar(int sock) {
    T value;
    if (recv(sock, &value, sizeof(T), 0) != sizeof(T)) {
        std::cerr << "could not recv: " << strerror(errno) << std::endl;
        throw std::runtime_error("");
    }

    // TODO: SELECT запрос обычный в самом начале ничего не возвращает (пустой), главное без ошибок
    // всяких

    return value;
}

void handleTuple(int sock, const std::vector<int> &lengths) {
    std::vector<std::string> attrs{lengths.size()};
    for (const auto length : lengths) {
        auto byte = recvByte(sock);
        if (byte == '0') {
            attrs.push_back("NULL");
            continue;
        }
        assert(byte == '1');

        std::string val;
        switch (length) {
        case 8:
            val = std::to_string(readScalar<int64_t>(sock));
            break;
        case 4:
            val = std::to_string(readScalar<int32_t>(sock));
            break;
        case 2:
            val = std::to_string(readScalar<int16_t>(sock));
            break;
        case 1:
            val = std::to_string(readScalar<int8_t>(sock));
            break;
        case -1:
            val = receiveString(sock);
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

void sendSelect(int sock) {
    sendByte(sock, 'S');

    auto natts = recvTupleDescriptor(sock);

    auto stop = false;
    while (!stop) {
        auto byte = recvByte(sock);
        switch (byte) {
        case 'T':
            handleTuple(sock, natts);
            break;
        case 'E':
            stop = true;
            break;
        case 'S': {
            auto s = receiveString(sock);
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

void sendInsert(int sock) {
    // Пока только 2 числа есть в кортеже
    int32_t first;
    int16_t second;

    std::cin >> first >> second;

    auto buffer = std::vector<char>(sizeof(char) + sizeof(int32_t) + sizeof(int16_t));

    auto cursor = buffer.data();

    // Command type 'I'nsert
    *cursor = 'I';
    cursor += sizeof(char);

    *reinterpret_cast<uint32_t *>(cursor) = htonl(static_cast<uint32_t>(first));
    cursor += sizeof(int32_t);

    *reinterpret_cast<uint16_t *>(cursor) = htons(static_cast<uint16_t>(second));

    auto left = static_cast<size_t>(buffer.size());
    cursor = buffer.data();
    while (left > 0) {
        auto ret = send(sock, cursor, left, 0);
        if (send(sock, cursor, left, 0) < 0) {
            std::cerr << "could not send: " << strerror(errno) << std::endl;
            throw std::runtime_error("");
        }

        left -= static_cast<size_t>(ret);
        cursor += ret;
    }

    auto ret = recvByte(sock);
    if (ret == 'K') {
        // OK
    } else if (ret == 'S') {
        auto str = receiveString(sock);
        std::cerr << "ERROR: " << str << std::endl;
    } else {
        std::cerr << "Unknown result byte " << ret << std::endl;
    }
}

int main() {
    auto client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        std::cerr << "could not socket: " << strerror(errno) << std::endl;
        return 1;
    }

    auto addr = sockaddr_in{.sin_family = AF_INET,
                            .sin_port = htons(6543),
                            .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)},
                            .sin_zero = {}};

    if (connect(client, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "could not connect: " << strerror(errno) << std::endl;
        return -1;
    }

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

    shutdown(client, SHUT_RDWR);
    close(client);
    return 0;
}