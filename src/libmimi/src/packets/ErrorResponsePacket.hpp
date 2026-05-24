#pragma once

#include "packets/IPacket.hpp"
#include <string>
namespace mi::interface::libmimi {
class ErrorResponsePacket : public IPacket {
  private:
    std::string _message;

  public:
    ErrorResponsePacket(std::string message)
        : IPacket(PacketType::ErrorResponse), _message(std::move(message)) {};
    const std::string &Message() const { return this->_message; }

    void Accept(IPacketVisitor &visitor) const override;
};
} // namespace mi::interface::libmimi
