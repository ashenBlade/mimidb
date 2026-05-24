#pragma once

#include "packets/PacketType.hpp"

namespace mi::interface::libmimi {

class IPacketVisitor;

class IPacket {
  protected:
    PacketType _type;

  public:
    IPacket(PacketType type) : _type(type) {};
    PacketType Type() const { return this->_type; };

    virtual void Accept(IPacketVisitor &visitor) const = 0;
    virtual ~IPacket() = default;
};

} // namespace mi::interface::libmimi
