#pragma once

#include <cstdint>

namespace mi::storage {
struct PageNumber {
    uint32_t value;

    PageNumber(uint32_t value) : value(value) {};
    operator uint32_t() { return value; };

    bool isValid() const { return value != PageNumber::Invalid; };

    /// @brief Invalid page number
    static constexpr const uint32_t Invalid = UINT32_MAX;
    /// @brief Maximal page number
    static constexpr const uint32_t Max = UINT32_MAX - 1;
};
} // namespace mi::storage
