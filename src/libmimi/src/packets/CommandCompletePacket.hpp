#pragma once

#include "packets/IPacket.hpp"
namespace mi::interface::libmimi {
class CommandCompletePacket : public IPacket {
  public:
    CommandCompletePacket() : IPacket(PacketType::CommandComplete) {};
    void Accept(IPacketVisitor &visitor) const;
};
} // namespace mi::interface::libmimi
