#include "MimiClient.hpp"
#include "NetworkReader.hpp"
#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/IPacket.hpp"
#include "packets/PacketDeserializer.hpp"
#include "packets/PacketType.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/SerializerPacketVisitor.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <array>
#include <assert.h>
#include <cstddef>
#include <cstring>
#include <endian.h>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace mi::interface::libmimi;

extern MimiClient::MimiClient(int sock) : _socket(sock){};

extern void MimiClient::sendBuffer(const std::byte *buffer, size_t length) {
    auto left = static_cast<ssize_t>(length);
    auto cursor = buffer;
    while (left > 0) {
        auto ret = send(this->_socket, cursor, static_cast<size_t>(left), 0);
        if (ret < 0) {
            throw std::runtime_error("could not send");
        }
        left -= static_cast<ssize_t>(ret);
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
        if (ret <= 0) {
            throw std::runtime_error("could not recv");
        }
        left -= static_cast<ssize_t>(ret);
        cursor += ret;
    }
}

bool MimiClient::recvBufferOpt(std::byte *buffer, size_t length) {
    auto left = static_cast<ssize_t>(length);
    auto cursor = buffer;
    while (left > 0) {
        auto ret = recv(this->_socket, cursor, static_cast<size_t>(left), 0);
        if (ret == 0) {
            return false;
        } else if (ret < 0) {
            std::stringstream str{"could not recv: "};
            str << strerror(errno);
            throw std::runtime_error(str.str());
        }
        left -= static_cast<ssize_t>(ret);
        cursor += ret;
    }

    return true;
}

extern std::optional<int8_t> MimiClient::ReceiveInt8Opt() {
    int8_t value;
    if (this->recvBufferOpt(reinterpret_cast<std::byte *>(&value), sizeof(value))) {
        return value;
    } else {
        return std::nullopt;
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
    int32_t value;
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

extern void MimiClient::SendPacket(const IPacket &packet) {
    auto serializer = SerializerPacketVisitor{};
    packet.Accept(serializer);

    auto buffer = std::move(serializer._buffer);
    if (buffer.size() <= 1) {
        throw std::runtime_error("could not serialize packet");
    }

    this->sendBuffer(buffer.data(), buffer.size());
}

extern std::unique_ptr<IPacket> MimiClient::ReceivePacket() {
    // Check header is valid
    auto byte = this->ReceiveInt8Opt();
    if (!byte.has_value()) {
        return nullptr;
    }

    auto type = static_cast<PacketType>(byte.value());
    switch (type) {
    case PacketType::DataRow:
    case PacketType::ErrorResponse:
    case PacketType::Query:
    case PacketType::TupleDescription:
    case PacketType::CommandComplete:
        // valid
        break;
    default:
        throw std::runtime_error("Unknown packet type: " + std::to_string(static_cast<int>(type)));
    }

    auto length = static_cast<uint32_t>(this->ReceiveInt32());
    // Length of 'length' field is included
    if (length < sizeof(uint32_t)) {
        throw std::runtime_error("invalid length field value in packet: " + std::to_string(length));
    }

    auto buffer = std::vector<std::byte>(length - sizeof(uint32_t));
    this->ReceiveBuffer(buffer.data(), buffer.size());

    std::unique_ptr<IPacket> packet = nullptr;
    switch (type) {
    case PacketType::DataRow: {
        packet = std::make_unique<DataRowPacket>(
            PacketDeserializer::DeserializeDataRow(buffer.data(), buffer.size()));
        break;
    }
    case PacketType::ErrorResponse: {
        packet = std::make_unique<ErrorResponsePacket>(
            PacketDeserializer::DeserializeErrorResponse(buffer.data(), buffer.size()));
        break;
    }
    case PacketType::Query: {
        packet = std::make_unique<QueryPacket>(
            PacketDeserializer::DeserializeQuery(buffer.data(), buffer.size()));
        break;
    }
    case PacketType::TupleDescription: {
        packet = std::make_unique<TupleDescriptionPacket>(
            PacketDeserializer::DeserializeTupleDescription(buffer.data(), buffer.size()));
        break;
    }
    case PacketType::CommandComplete: {
        packet = std::make_unique<CommandCompletePacket>(
            PacketDeserializer::DeserializeCommandComplete(buffer.data(), buffer.size()));
        break;
    }
    }

    assert(packet != nullptr);
    return packet;
}

extern MimiClient::~MimiClient() { this->Close(); }
