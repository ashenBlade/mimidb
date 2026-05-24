#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace mi::interface::libmimi {
class NetworkReader {
  private:
    // Pointer to current position in buffer
    std::byte *_cursor;
    // Index in buffer of 'cursor'
    size_t _pos;
    // Total size of whole buffer
    size_t _max;
    void checkBufferSize(size_t delta);

  public:
    NetworkReader(std::byte *buffer, size_t length);
    int8_t ReadInt8();
    uint8_t ReadUint8();
    uint32_t ReadUint32();
    int32_t ReadInt32();
    std::string ReadString();
    // Read continuous memory space of length 'length' and store it into 'buffer'
    void ReadBuffer(size_t length, std::byte *buffer);
};
} // namespace mi::common
