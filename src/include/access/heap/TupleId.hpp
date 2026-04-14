#pragma once

#include <cstdint>

#include "storage/PageNumber.hpp"

namespace mi::access::heap {
struct TupleId final {
    // Number of page in table
    mi::storage::PageNumber pageno;
    // Index of item in page directory
    uint16_t itemid;

    TupleId() : pageno(storage::PageNumber::Invalid), itemid(0) {};
    TupleId(mi::storage::PageNumber pageno, uint16_t itemid) : pageno(pageno), itemid(itemid) {};
};
}; // namespace mi::access::heap
