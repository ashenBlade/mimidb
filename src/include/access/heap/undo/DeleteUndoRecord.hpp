#pragma once

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"

namespace mi::access::heap::undo {
struct DeleteUndoRecord : public HeapUndoRecordBase {
    // Tuple that was deleted
    HeapPageTupleHeader Tuple;

    DeleteUndoRecord() : HeapUndoRecordBase(HeapUndoRecordType::Delete), Tuple() {};
    ~DeleteUndoRecord() {};
};
} // namespace mi::access::heap::undo
