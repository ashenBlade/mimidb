#pragma once

#include <cstdint>

#include "storage/undo/UndoSeqNumber.hpp"
#include "trans/TransactionId.hpp"

namespace mi::access::heap {
enum HeapTupleFlags : uint64_t {
    Deleted = 1U << 0,  // Tuple was deleted or updated, but new tuple version is in another place
                        // (see undo log record to know what happened).
    HasNulls = 1U << 1, // Some attributes are nulls
};

struct HeapPageTupleHeader {
    // Id of transaction created this tuple
    storage::trans::TransactionId xid;
    // Location of undo record for this tuple
    storage::undo::UndoSeqNumber undo;
    // Special flags for tuple
    HeapTupleFlags flags;

    // Data follows header without padding, because flags is 8 bytes,
    // so header is aligned by 8.
};

}; // namespace mi::access::heap