#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace mi::interface::libmimi {
class NetworkWriter {
  private:
    // Pointer to current position in buffer
    std::byte *_cursor;
    // Index in buffer of 'cursor'
    size_t _pos;
    // Total size of whole buffer
    size_t _max;
    void checkBufferSize(size_t delta);

  public:
    NetworkWriter(std::byte *buffer, size_t length);
    NetworkWriter(): NetworkWriter(nullptr, 0) {};
    void WriteInt8(int8_t value);
    void WriteUint8(uint8_t value);
    void WriteUint32(uint32_t value);
    void WriteInt32(int32_t value);
    void WriteString(const std::string &value);
    void WriteBuffer(const std::byte *buffer, size_t length);
};
} // namespace mi::common
