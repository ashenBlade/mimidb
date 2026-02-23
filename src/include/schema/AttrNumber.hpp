#pragma once

#include <cstdint>

namespace mi::schema {

/// @brief Number of attribute in tuple
struct AttrNumber {
    int16_t value;

    AttrNumber(int16_t value): value(value) { };
    operator int16_t() const { return value; };

    /// @brief Minimal value for attribute number
    static constexpr const int16_t Min = 1;
    /// @brief Max value for attribute number
    static constexpr const int16_t Max = INT16_MAX;
    /// @brief Invalid value for attribute number
    static constexpr const int16_t Invalid = 0;
};

};