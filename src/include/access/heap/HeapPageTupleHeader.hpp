#pragma once

#include "transam/TransactionId.hpp"
#include "transam/UndoSeqNumber.hpp"

namespace mi::access::heap {
enum class HeapTupleFlags : uint8_t {
    Deleted = 0x1,
};

struct HeapPageTupleHeader {
    /// @brief Id of transaction created this tuple
    mi::transam::TransactionId xid;
    /// @brief Location of undo record for this tuple
    mi::transam::UndoSeqNumber undo;
    /// @brief Special flags for tuple
    HeapTupleFlags flags;
};
}; // namespace mi::access::heap