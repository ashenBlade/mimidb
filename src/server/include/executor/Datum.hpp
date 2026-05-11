#pragma once

#include <cstddef>

namespace mi {

/// @brief Datum represents arbitrary value. There is no type information and
///        it must be known to caller
union Datum final {
  private:
    /// @brief Actual value of datum stored as some scalar large enough to store both largest scalar
    /// type and pointer
    std::ptrdiff_t _value;

  public:
    explicit Datum() : _value(0) {};
    template <class T> explicit Datum(T *value) : _value(reinterpret_cast<std::ptrdiff_t>(value)) {}
    template <class T> explicit Datum(T value) : _value(static_cast<std::ptrdiff_t>(value)) {}

    Datum(const Datum &other) noexcept = default;
    Datum &operator=(const Datum &other) noexcept = default;
    Datum(Datum &&other) noexcept = default;
    Datum &operator=(Datum &&other) noexcept = default;

    template <class T> T getScalar() const { return static_cast<T>(this->_value); }

    template <class T> T *getPointer() const { return reinterpret_cast<T *>(this->_value); }
};

}; // namespace mi
