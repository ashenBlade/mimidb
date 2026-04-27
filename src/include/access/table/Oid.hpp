#pragma once

#include <cstdint>
// keep for std::hash impl
#include <unordered_map>

namespace mi {

struct Oid {
    using type = uint32_t;

    uint32_t value;

    Oid(): value(Oid::Invalid) {};
    Oid(uint32_t value) : value(value) {};
    bool IsValid() const { return this->value != Oid::Invalid; };

    explicit operator uint32_t() const { return this->value; };
    bool operator==(const Oid &other) const noexcept { return this->value == other.value; }

    /// @brief Invalid value of Oid
    static constexpr const uint32_t Invalid = 0;
};

}; // namespace mi::access::table

namespace std {

template <> struct hash<mi::Oid> {
    size_t operator()(const mi::Oid &oid) const {
        return std::hash<int>()(static_cast<int>(oid.value));
    }
};

}; // namespace std