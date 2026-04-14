#pragma once

#include <cassert>
#include <cstdint>

namespace mi::access::table {

/// @brief Number of attribute in tuple
struct AttrNumber {
    int16_t value;

    AttrNumber(int16_t value) : value(value) {};
    int16_t ToIndex() const {
        assert(this->value != AttrNumber::Invalid);
        return this->value - 1;
    }

    operator int16_t() const { return value; };

    /// @brief Minimal value for attribute number
    static constexpr const int16_t Min = 1;
    /// @brief Max value for attribute number
    static constexpr const int16_t Max = INT16_MAX;
    /// @brief Invalid value for attribute number
    static constexpr const int16_t Invalid = 0;
};

}; // namespace mi::schema