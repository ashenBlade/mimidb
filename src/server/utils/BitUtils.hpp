#pragma once

#include <cstddef>

namespace mi {
class BitUtils final {
  public:
    template <class T> static constexpr T TypeAlign(T length, size_t alignment) {
        return static_cast<T>((length + (alignment - 1)) & ~(alignment - 1));
    }
    template <class T> static constexpr T MaxAlign(T length) {
        return TypeAlign(length, alignof(size_t));
    }
    template <class T> static constexpr size_t BitmapSize(T size) { return (size + 7) / 8; }
};

} // namespace mi