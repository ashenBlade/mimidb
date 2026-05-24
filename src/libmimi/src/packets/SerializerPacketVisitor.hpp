#pragma once

#include "IPacketVisitor.hpp"
#include "packets/CommandCompletePacket.hpp"
#include <cstddef>
#include <vector>
namespace mi::interface::libmimi {

class SerializerPacketVisitor : public IPacketVisitor {
  private:
  public:
    std::vector<std::byte> _buffer;

  public:
    void Visit(const QueryPacket &packet) override;
    void Visit(const TupleDescriptionPacket &packet) override;
    void Visit(const DataRowPacket &packet) override;
    void Visit(const ErrorResponsePacket &packet) override;
    void Visit(const CommandCompletePacket &packet) override;

    ~SerializerPacketVisitor() override = default;
};

} // namespace mi::interface::libmimi
