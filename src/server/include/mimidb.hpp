#pragma once

#include <cassert>
#include <cstdint>

namespace mi {

// Size of table page, 8KB
constexpr int PAGESIZE = 8192;

constexpr uint32_t BITS_PER_BYTE = 8;

}; // namespace mi

#define WITH(expr) if (expr; true)
