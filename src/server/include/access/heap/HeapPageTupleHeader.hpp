#pragma once

#include <cstdint>

#include "trans/TransactionId.hpp"
#include "storage/undo/UndoSeqNumber.hpp"

namespace mi::access::heap {
enum HeapTupleFlags : uint8_t {
    Deleted = 1U, // Tuple was deleted or updated, but new tuple version is in another place (see undo log record to know what happened).
    HasNulls = 1U << 1, // Some attributes are nulls
};

struct HeapPageTupleHeader {
    /// @brief Id of transaction created this tuple
    transam::TransactionId xid;
    /// @brief Location of undo record for this tuple
    transam::UndoSeqNumber undo;
    /// @brief Special flags for tuple
    HeapTupleFlags flags;
    /// @brief Offset to start of data
    uint8_t dataStartOffset;
};

}; // namespace mi::access::heap