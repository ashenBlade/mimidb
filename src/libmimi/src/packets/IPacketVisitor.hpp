#pragma once

#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
namespace mi::interface::libmimi {

class IPacketVisitor {
  public:
    virtual void Visit(const QueryPacket &packet) = 0;
    virtual void Visit(const CommandCompletePacket &packet) = 0;
    virtual void Visit(const TupleDescriptionPacket &packet) = 0;
    virtual void Visit(const DataRowPacket &packet) = 0;
    virtual void Visit(const ErrorResponsePacket &packet) = 0;
    virtual ~IPacketVisitor() = default;
};

} // namespace mi::interface::libmimi
