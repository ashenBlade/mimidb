#pragma once

#include <cstdint>

#include "storage/PageNumber.hpp"

namespace mi::access::heap {
struct TupleId final {
    /// @brief Number of page in table
    mi::storage::PageNumber pageno;
    /// @brief Index of item in page directory
    uint16_t itemid;
};
}; // namespace mi::access::heap
