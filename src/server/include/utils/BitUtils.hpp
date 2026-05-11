#pragma once

#include <cstddef>

namespace mi {

// TODO: это все переделать на статические функции

template <class T> constexpr T TypeAlign(T length, size_t alignment) {
    return static_cast<T>((length + (alignment - 1)) & ~(alignment - 1));
}

template <class T> constexpr T MaxAlign(T length) { return TypeAlign(length, alignof(size_t)); }

template <class T> constexpr size_t BitmapSize(T size) { return (size + 7) / 8; }

} // namespace mi