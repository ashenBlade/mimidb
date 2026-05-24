#pragma once

#include "packets/IPacket.hpp"
#include <optional>
#include <string>
#include <vector>
namespace mi::interface::libmimi {
class DataRowPacket : public IPacket {
  private:
    std::vector<std::optional<std::string>> _values;

  public:
    DataRowPacket(std::vector<std::optional<std::string>> values)
        : IPacket(PacketType::DataRow), _values(std::move(values)) {};
    const std::vector<std::optional<std::string>> &Values() const { return this->_values; };

    void Accept(IPacketVisitor &visitor) const override;
};
} // namespace mi::interface::libmimi
