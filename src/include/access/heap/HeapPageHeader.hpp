#pragma once

#include <cstdint>

#include "transam/LogSeqNumber.hpp"
#include "transam/TransactionId.hpp"

namespace mi::access::heap {

/// @brief Header of heap page on disk
struct HeapPageHeader final {
    // Last applied changes to the page.
    // Must be first on page.
    mi::transam::LogSeqNumber lsn;
    // Amount of free bytes on occupied part of page.
    // Creates during update/delete operations.
    uint16_t  fragmented;
    // Start of free space
    uint16_t  lower;
    // End of free space
    uint16_t  upper;
};

}; // namespace mi::access::heap
