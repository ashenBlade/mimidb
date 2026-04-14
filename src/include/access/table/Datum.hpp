#pragma once

#include <cstddef>

namespace mi::access::table {

/// @brief Datum represents arbitrary value. There is no type information and
///        it must be known to caller
union Datum final {
  private:
    /// @brief Pointer value
    void *_pointer;
    /// @brief Integer value
    std::ptrdiff_t _integer;

  public:
    template <class T> Datum(T *value) : _pointer(static_cast<void *>(value)){};

    template <class T> Datum(T value) : _integer(static_cast<std::ptrdiff_t>(value)){};

    template <class T> T getInt() const { return static_cast<T>(_integer); }

    template <class T> T *getPointer() const { return static_cast<T *>(_pointer); }
};

}; // namespace mi::access::table
