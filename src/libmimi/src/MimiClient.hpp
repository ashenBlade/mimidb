#pragma once

#include "packets/IPacket.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mi::interface::libmimi {
class MimiClient final {
  private:
    // Socket for communication
    int _socket;

    void sendBuffer(const std::byte *buffer, size_t length);
    void recvBuffer(std::byte *buffer, size_t length);
    bool recvBufferOpt(std::byte *buffer, size_t length);

    // Send functions
    void SendInt8(int8_t value);
    void SendInt16(int16_t value);
    void SendInt32(int32_t value);
    void SendInt64(int64_t value);
    void SendBuffer(const std::byte *buffer, size_t length);
    void SendBuffer(const std::string &buffer);

    // Same as SendBuffer, but also sends length as int32_t
    void SendString(const std::byte *buffer, size_t length);
    void SendString(const std::string &buffer);

  public:
    MimiClient(int sock);

    MimiClient(const MimiClient &) = delete;
    MimiClient &operator=(const MimiClient &) = delete;

    // Receive functions
    std::optional<int8_t> ReceiveInt8Opt();
    int8_t ReceiveInt8();
    int16_t ReceiveInt16();
    int32_t ReceiveInt32();
    int64_t ReceiveInt64();
    void ReceiveBuffer(std::byte *buffer, size_t length);
    std::string ReceiveString();

    // Returns next packet from underlying connection.
    // If there is no more data (i.e. connection closed), then nullptr is returned.
    std::unique_ptr<IPacket> ReceivePacket();
    void SendPacket(const IPacket &packet);

    // Close connection
    void Close();

    ~MimiClient();
};
} // namespace mi::interface::libmimi
