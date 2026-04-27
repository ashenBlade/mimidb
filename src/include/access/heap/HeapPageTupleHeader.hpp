#pragma once

#include <cstddef>

#include "transam/CommitSeqNumber.hpp"
#include "transam/TransactionId.hpp"
#include "transam/UndoSeqNumber.hpp"

namespace mi::access::heap {
enum HeapTupleFlags : uint8_t {
    Deleted = 0x1,  // Tuple was deleted, so no new updated versions exist and no data exists on page
    HasNulls = 0x2, // Some attributes are nulls
};

struct HeapPageTupleHeader {
    /// @brief Id of transaction created this tuple
    transam::TransactionId xid;
    /// @brief Location of undo record for this tuple
    transam::UndoSeqNumber undo;
    /// @brief Special flags for tuple
    HeapTupleFlags flags;
    /// @brief Offset to start of data
    uint8_t hoff;
    
    
};

constexpr std::size_t SizeofHeapPageTupleHeader = offsetof(HeapPageTupleHeader, hoff) + sizeof(uint8_t);

}; // namespace mi::access::heap