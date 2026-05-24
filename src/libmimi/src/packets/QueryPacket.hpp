#pragma once

#include "packets/IPacket.hpp"
#include <string>
namespace mi::interface::libmimi {
class QueryPacket : public IPacket {
  private:
    std::string _query;

  public:
    QueryPacket(std::string query) : IPacket(PacketType::Query), _query(std::move(query)) {};
    const std::string &Query() const { return this->_query; };
    void Accept(IPacketVisitor &visitor) const override;
};
} // namespace mi::interface::libmimi
