#pragma once

#include <bits/functional_hash.h>
#include <cstdint>

namespace mi::storage::buffer {
struct PageNumber {
    // Type used for page numbers
    using type = uint32_t;

    type value;

    PageNumber() : value(PageNumber::Invalid) {};
    PageNumber(type value) : value(value) {};
    bool IsValid() const { return this->value != PageNumber::Invalid; };

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
    static constexpr const type Invalid = UINT32_MAX;
    // First valid page number
    static constexpr const type Min = 0;
    // Maximal page number
    static constexpr const type Max = UINT32_MAX - 1;
};
} // namespace mi::storage::buffer

namespace std {

template <> struct hash<mi::storage::buffer::PageNumber> {
    size_t operator()(const mi::storage::buffer::PageNumber &pageno) {
        return std::hash<int>()(static_cast<int>(pageno.value));
    }
};

}; // namespace std
