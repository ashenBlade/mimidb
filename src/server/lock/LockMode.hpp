#pragma once

#include <cstdint>

namespace mi::lock {
enum class LockMode : std::uint8_t {
    // Lock in share mode
    Share = 0,
    Exclusive = 1,
};
}
