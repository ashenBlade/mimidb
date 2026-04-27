#pragma once

#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"

namespace mi::access::heap::undo {
// Tuple was inserted
struct InsertUndoRecord : public HeapUndoRecordBase {
    uint32_t TupleLen;
    HeapPageTupleHeader Tuple;

    InsertUndoRecord() : HeapUndoRecordBase(HeapUndoRecordType::Insert) { };
    ~InsertUndoRecord() { };
};
} // namespace mi::access::heap::undo