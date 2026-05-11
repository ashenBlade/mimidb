#pragma once

#include <cstdint>
#include <cstddef>

#include "storage/wal/LogSeqNumber.hpp"

namespace mi::access::heap {

// Header of heap page on disk, without item array
struct HeapPageHeader final {
    // Last applied changes to the page.
    // Must be first on page.
    mi::transam::LogSeqNumber lsn;
    // Start of free space
    uint16_t lower;
    // End of free space
    uint16_t upper;
};

constexpr std::size_t SizeOfHeapPageHeader = offsetof(HeapPageHeader, upper) + sizeof(uint16_t);

}; // namespace mi::access::heap
