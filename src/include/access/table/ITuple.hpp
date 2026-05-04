#pragma once

#include <optional>

#include "access/table/Datum.hpp"
#include "access/table/AttrNumber.hpp"

namespace mi::access::table {

/// @brief Runtime tuple representation
class ITuple {
  public:
    /// @brief Get attribute value at specified number
    /// @param attrNumber Number of attribute
    /// @return Value of attribute or nullopt if it is null
    virtual std::optional<Datum> GetAttribute(AttrNumber attrNumber) = 0;

    virtual ~ITuple() = default;
};

}; // namespace mi::access::table