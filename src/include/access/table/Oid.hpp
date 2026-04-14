#pragma once

#include <cstdint>

namespace mi::access::table {

struct Oid {
    uint32_t value;

    Oid(uint32_t value) : value(value) {};
    operator uint32_t() const { return this->value; };
    bool IsValid() const { return this->value != Oid::Invalid; };

    /// @brief Invalid value of Oid
    static constexpr const uint32_t Invalid = 0;
};

}; // namespace mi::access::table