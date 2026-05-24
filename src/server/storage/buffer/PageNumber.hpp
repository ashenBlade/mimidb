#pragma once

#include <bits/functional_hash.h>
#include <cstdint>

namespace mi::storage::buffer {
struct PageNumber {
  private:
    // Invalid page number
    static constexpr const uint32_t InvalidValue = UINT32_MAX;
    // First valid page number
    static constexpr const uint32_t MinValue = 0;
    // Maximal page number
    static constexpr const uint32_t MaxValue = UINT32_MAX - 1;

  public:
    using type = uint32_t;

    uint32_t value;

    PageNumber() : value(InvalidValue) {};
    PageNumber(uint32_t value) : value(value) {};
    bool IsValid() const { return this->value != InvalidValue; };

    operator uint32_t() { return this->value; };
    bool operator==(const PageNumber &other) const noexcept { return this->value == other.value; }
    template <class T> bool operator==(T value) const noexcept { return this->value == value; }

    template <class T> PageNumber operator+(T value) { return PageNumber{this->value + value}; }

    PageNumber &operator++() {
        this->value++;
        return *this;
    }

    PageNumber operator++(int) {
        auto copy = *this;
        this->value++;
        return copy;
    }

    // Invalid page number
    static PageNumber Min() { return PageNumber{MinValue}; }
    // First valid page number
    static PageNumber Max() { return PageNumber{MaxValue}; }
    // Maximal page number
    static PageNumber Invalid() { return PageNumber{InvalidValue}; };
};
} // namespace mi::storage::buffer

namespace std {

template <> struct hash<mi::storage::buffer::PageNumber> {
    size_t operator()(const mi::storage::buffer::PageNumber &pageno) {
        return std::hash<int>()(static_cast<int>(pageno.value));
    }
};

}; // namespace std
