#include "NetworkReader.hpp"
#include <cstring>
#include <stdexcept>

using namespace mi::interface::libmimi;

NetworkReader::NetworkReader(std::byte *buffer, size_t length)
    : _cursor(buffer), _pos(0), _max(length) {};

void NetworkReader::checkBufferSize(size_t delta) {
    if (this->_max < this->_pos + delta) {
        throw std::runtime_error("could not read past buffer borders");
    }
}

uint8_t NetworkReader::ReadUint8() {
    this->checkBufferSize(sizeof(uint8_t));
    auto value = *reinterpret_cast<uint8_t *>(this->_cursor);
    this->_cursor += sizeof(uint8_t);
    this->_pos += sizeof(uint8_t);
    return value;
}

int8_t NetworkReader::ReadInt8() {
    this->checkBufferSize(sizeof(int8_t));
    auto value = *reinterpret_cast<int8_t *>(this->_cursor);
    this->_cursor += sizeof(int8_t);
    this->_pos += sizeof(int8_t);
    return value;
}

uint32_t NetworkReader::ReadUint32() {
    this->checkBufferSize(sizeof(uint32_t));
    auto value = be32toh(*reinterpret_cast<uint32_t *>(this->_cursor));
    this->_cursor += sizeof(uint32_t);
    this->_pos += sizeof(uint32_t);
    return value;
}

int32_t NetworkReader::ReadInt32() {
    this->checkBufferSize(sizeof(int32_t));
    auto value = be32toh(*reinterpret_cast<uint32_t *>(this->_cursor));
    this->_cursor += sizeof(int32_t);
    this->_pos += sizeof(int32_t);
    return static_cast<int32_t>(value);
}

std::string NetworkReader::ReadString() {
    auto len = this->ReadUint32();
    auto str = std::string(len, 0);
    this->ReadBuffer(len, reinterpret_cast<std::byte *>(str.data()));
    return str;
}

void NetworkReader::ReadBuffer(size_t length, std::byte *buffer) {
    this->checkBufferSize(length);
    std::memcpy(buffer, this->_cursor, length);
    this->_cursor += length;
    this->_pos += length;
}
