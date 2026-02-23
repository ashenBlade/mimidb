#pragma once

#include <optional>

#include "schema/AttrNumber.hpp"
#include "executor/Datum.hpp"

namespace mi::executor {

/// @brief Runtime tuple representation
class ITuple {
public:
    /// @brief Get attribute value at specified number
    /// @param attrNumber Number of attribute
    /// @param isnull This attribute is null
    /// @return Value of attribute or 
    virtual std::optional<Datum> GetAttribute(mi::schema::AttrNumber attrNumber, bool &isnull) = 0;
};

};