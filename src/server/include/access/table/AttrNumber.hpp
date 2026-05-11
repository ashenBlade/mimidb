#pragma once

#include <cassert>
#include <cstdint>

namespace mi::access::table {

/// @brief Number of attribute in tuple
struct AttrNumber {
    using type = uint16_t;

    uint16_t value;

    AttrNumber() : value(Invalid) {};
    AttrNumber(uint16_t value) : value(value) {};
    constexpr std::size_t ToIndex() const {
        assert(this->value != AttrNumber::Invalid);
        return static_cast<std::size_t>(this->value - 1);
    }

    operator std::size_t() const { return value; };
    operator uint16_t() const { return value; }

    template<class T>
    bool operator==(T other) {
        return this->value == other;
    }

    bool operator==(const AttrNumber &other) {
        return this->value == other.value;
    }

    bool operator<=(const AttrNumber &other) const {
        return this->value <= other.value;
    }
    
    bool operator<(const AttrNumber &other) const {
        return this->value < other.value;
    }

    AttrNumber &operator++() {
        this->value++;
        return *this;
    }
    
    AttrNumber operator++(int) {
        auto copy = *this;
        this->value++;
        return copy;
    }

    /// @brief Minimal value for attribute number
    static constexpr const uint16_t Min = 1;
    /// @brief Max value for attribute number
    static constexpr const uint16_t Max = UINT16_MAX;
    /// @brief Invalid value for attribute number
    static constexpr const uint16_t Invalid = 0;
};

}; // namespace mi::access::table