#pragma once

#include <cassert>
#include <cstdint>

namespace mi::access::table {

/// @brief Number of attribute in tuple
struct AttrNumber {
  private:
    static constexpr uint16_t InvalidValue = 0;
    static constexpr uint16_t MinValue = 1;
    static constexpr uint16_t MaxValue = UINT16_MAX;

  public:
    using type = uint16_t;

    uint16_t value;

    AttrNumber() : value(InvalidValue) {};
    AttrNumber(uint16_t value) : value(value) {};
    constexpr std::size_t ToIndex() const {
        assert(this->value != InvalidValue);
        return static_cast<std::size_t>(this->value - 1);
    }

    operator std::size_t() const { return value; };
    operator uint16_t() const { return value; }

    template <class T> bool operator==(T other) { return this->value == other; }
    bool operator==(const AttrNumber &other) { return this->value == other.value; }
    bool operator<=(const AttrNumber &other) const { return this->value <= other.value; }
    bool operator<(const AttrNumber &other) const { return this->value < other.value; }

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
    static AttrNumber Min() { return AttrNumber{MinValue}; }

    /// @brief Max value for attribute number
    static AttrNumber Max() { return AttrNumber{MaxValue}; }

    /// @brief Invalid value for attribute number
    static AttrNumber Invalid() { return AttrNumber{InvalidValue}; }
};

}; // namespace mi::access::table