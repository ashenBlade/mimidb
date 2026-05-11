#pragma once

#include <cstdint>
#include <cassert>

namespace mi {

// Size of table page, 8KB
constexpr int PAGESIZE = 8192;

constexpr uint32_t BITS_PER_BYTE = 8;

};


#define WITH(expr)      if (expr; true)
