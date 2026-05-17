#include "NetworkWriter.hpp"
#include <cstring>
#include <stdexcept>

using namespace mi::interface::libmimi;

NetworkWriter::NetworkWriter(std::byte *buffer, size_t length)
    : _cursor(buffer), _pos(0), _max(length) {};

void NetworkWriter::checkBufferSize(size_t delta) {
    if (this->_max < this->_pos + delta) {
        throw std::runtime_error("could not write past buffer borders");
    }
}

void NetworkWriter::WriteUint8(uint8_t value) {
    this->checkBufferSize(sizeof(uint8_t));
    *reinterpret_cast<uint8_t *>(this->_cursor) = value;
    this->_cursor += sizeof(uint8_t);
    this->_pos += sizeof(uint8_t);
}

void NetworkWriter::WriteInt8(int8_t value) {
    this->checkBufferSize(sizeof(int8_t));
    *reinterpret_cast<int8_t *>(this->_cursor) = value;
    this->_cursor += sizeof(int8_t);
    this->_pos += sizeof(int8_t);
}

void NetworkWriter::WriteUint32(uint32_t value) {
    this->checkBufferSize(sizeof(uint32_t));
    *reinterpret_cast<uint32_t *>(this->_cursor) = htobe32(value);
    this->_cursor += sizeof(uint32_t);
    this->_pos += sizeof(uint32_t);
}

void NetworkWriter::WriteInt32(int32_t value) {
    this->checkBufferSize(sizeof(int32_t));
    *reinterpret_cast<uint32_t *>(this->_cursor) = htobe32(static_cast<uint32_t>(value));
    this->_cursor += sizeof(int32_t);
    this->_pos += sizeof(int32_t);
}

void NetworkWriter::WriteString(const std::string &value) {
    this->WriteUint32(static_cast<uint32_t>(value.size()));
    this->WriteBuffer(reinterpret_cast<const std::byte *>(value.data()), value.size());
}

void NetworkWriter::WriteBuffer(const std::byte *buffer, size_t length) {
    this->checkBufferSize(length);
    std::memcpy(this->_cursor, buffer,  length);
    this->_cursor += length;
    this->_pos += length;
}
