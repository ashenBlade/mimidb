#pragma once

#include <cstdint>

namespace mi::storage {
struct PageNumber {
    using type = uint32_t;
    
    type value;

    PageNumber(): value (PageNumber::Invalid) {};
    PageNumber(type value) : value(value) {};
    operator type() { return this->value; };

    bool IsValid() const { return this->value != PageNumber::Invalid; };

    // Invalid page number
    static constexpr const type Invalid = UINT32_MAX;
    // First valid page number
    static constexpr const type Min = 0;
    // Maximal page number
    static constexpr const type Max = UINT32_MAX - 1;
};
} // namespace mi::storage
