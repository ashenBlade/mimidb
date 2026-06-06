#pragma once

#include "packets/DataRowPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <memory>
#include <vector>

namespace mi::client {
class IFormatter {
  public:
    virtual void
    Format(const interface::libmimi::TupleDescriptionPacket &descriptor,
           const std::vector<std::unique_ptr<interface::libmimi::DataRowPacket>> &rows) = 0;
    virtual ~IFormatter() = default;
};
} // namespace mi::client
