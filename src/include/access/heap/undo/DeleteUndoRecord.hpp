#pragma once

#include "access/heap/TupleId.hpp"
#include "access/heap/undo/HeapUndoRecordBase.hpp"

namespace mi::access::heap::undo {
struct DeleteUndoRecord : public HeapUndoRecordBase {
    // Identifier of tuple to delete
    TupleId TupId;

    DeleteUndoRecord(TupleId tupleid) : HeapUndoRecordBase(HeapUndoRecordType::Delete), TupId(tupleid) {};
    ~DeleteUndoRecord() {};
};
} // namespace mi::access::heap::undo
