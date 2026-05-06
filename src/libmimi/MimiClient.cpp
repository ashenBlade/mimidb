#include "libmimi/MimiClient.hpp"

#include <endian.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

using namespace mi::interface::libmimi;

extern MimiClient::MimiClient(int sock): _socket(sock) {};

extern void MimiClient::sendBuffer(const std::byte *buffer, size_t length) {
    auto left = static_cast<ssize_t>(length);
    auto cursor = buffer;
    while (left > 0) {
        auto ret = send(this->_socket, cursor, static_cast<size_t>(left), 0);
        if (ret < 0) {
            throw std::runtime_error("could not send");
        }
        left += static_cast<ssize_t>(ret);
        cursor += ret;
    }
}

extern void MimiClient::SendInt8(int8_t value) {
    this->sendBuffer(reinterpret_cast<std::byte *>(&value), sizeof(value));
}

extern void MimiClient::SendInt16(int16_t value) {
    auto val = htobe16(static_cast<uint16_t>(value));
    this->sendBuffer(reinterpret_cast<std::byte *>(&val), sizeof(value));
}

extern void MimiClient::SendInt32(int32_t value) {
    auto val = htobe32(static_cast<uint32_t>(value));
    this->sendBuffer(reinterpret_cast<std::byte *>(&val), sizeof(value));
}

extern void MimiClient::SendInt64(int64_t value) {
    auto val = htobe64(static_cast<uint64_t>(value));
    this->sendBuffer(reinterpret_cast<std::byte *>(&val), sizeof(value));
}

extern void MimiClient::SendBuffer(const std::byte *buffer, size_t length) {
    this->sendBuffer(buffer, length);
}

extern void MimiClient::SendBuffer(const std::string &buffer) {
    this->sendBuffer(reinterpret_cast<const std::byte *>(buffer.data()), buffer.size());
}

extern void MimiClient::SendString(const std::string &string) {
    this->SendInt32(static_cast<int32_t>(string.size()));
    this->sendBuffer(reinterpret_cast<const std::byte *>(string.data()), string.size());
}

extern void MimiClient::recvBuffer(std::byte *buffer, size_t length) {
    auto left = static_cast<ssize_t>(length);
    auto cursor = buffer;
    while (left > 0) {
        auto ret = recv(this->_socket, cursor, static_cast<size_t>(left), 0);
        if (ret < 0) {
            throw std::runtime_error("could not recv");
        }
        left -= static_cast<ssize_t>(ret);
        cursor += ret;
    }
}

extern int8_t MimiClient::ReceiveInt8() {
    int8_t value;
    this->recvBuffer(reinterpret_cast<std::byte *>(&value), sizeof(value));
    return value;
}

extern int16_t MimiClient::ReceiveInt16() {
    int16_t value;
    this->recvBuffer(reinterpret_cast<std::byte *>(&value), sizeof(value));
    return static_cast<int16_t>(be16toh(static_cast<uint16_t>(value)));
}

extern int32_t MimiClient::ReceiveInt32() {
    int8_t value;
    this->recvBuffer(reinterpret_cast<std::byte *>(&value), sizeof(value));
    return static_cast<int32_t>(be32toh(static_cast<uint32_t>(value)));
}

extern int64_t MimiClient::ReceiveInt64() {
    int64_t value;
    this->recvBuffer(reinterpret_cast<std::byte *>(&value), sizeof(value));
    return static_cast<int64_t>(be64toh(static_cast<uint64_t>(value)));
}

extern void MimiClient::ReceiveBuffer(std::byte *buffer, size_t length) {
    this->recvBuffer(buffer, length);
}

extern std::string MimiClient::ReceiveString() {
    auto length = static_cast<size_t>(this->ReceiveInt32());
    auto str = std::string(length, 0);
    this->ReceiveBuffer(reinterpret_cast<std::byte *>(str.data()), length);
    return str;
}

extern void MimiClient::Close() {
    if (this->_socket == -1) {
        return;
    }
    
    shutdown(this->_socket, SHUT_RDWR);
    close(this->_socket);
    this->_socket = -1;
}

extern MimiClient::~MimiClient() {
    this->Close();
}
