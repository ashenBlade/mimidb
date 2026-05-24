#pragma once

#include "packets/IPacket.hpp"
#include <string>
#include <vector>
namespace mi::interface::libmimi {
class AttributeDescription {
  private:
    std::string _name;

  public:
    AttributeDescription(std::string name) : _name(std::move(name)) {};
    const std::string &Name() const { return this->_name; };
};

class TupleDescriptionPacket : public IPacket {
  private:
    std::vector<AttributeDescription> _attrs;

  public:
    TupleDescriptionPacket(std::vector<AttributeDescription> attrs)
        : IPacket(PacketType::TupleDescription), _attrs(std::move(attrs)) {};
    const std::vector<AttributeDescription> &Attributes() const { return this->_attrs; };
    void Accept(IPacketVisitor &visitor) const override;
};
} // namespace mi::interface::libmimi
