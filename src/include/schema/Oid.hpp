#pragma once

#include <cstdint>

namespace mi::schema {

struct Oid {
    uint32_t value;

    Oid(uint32_t value) : value(value) {};
    operator uint32_t() const { return value; };
    
    /// @brief Invalid value of Oid
    static constexpr const uint32_t Invalid = 0;
};

}; // namespace mi::schema