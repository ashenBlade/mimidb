#pragma once

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/TupleId.hpp"

namespace mi::access::heap::undo {
// Tuple was updated
struct UpdateUndoRecord : public HeapUndoRecordBase {
    // Position of old tuple
    TupleId From;
    // Position of new tuple
    TupleId To;
    
    // Length of tuple
    uint32_t TupleLen;

    // Old tuple. Must be last field.
    HeapPageTupleHeader Tuple;

    UpdateUndoRecord(): HeapUndoRecordBase(HeapUndoRecordType::Update), Tuple() {}
    ~UpdateUndoRecord() { };
};
} // namespace mi::access::heap::undo
