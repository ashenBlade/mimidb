#pragma once

namespace mi::access::heap::undo {
enum HeapUndoRecordType {
    Update = 1,
    Insert,
    Delete,
};

// Base class for all heap undo records
struct HeapUndoRecordBase {
    HeapUndoRecordType RecordType;
    
    HeapUndoRecordBase(HeapUndoRecordType type): RecordType(type) {};

    HeapUndoRecordBase(const HeapUndoRecordBase &other) = default;
    HeapUndoRecordBase &operator=(const HeapUndoRecordBase &other) = default;
    HeapUndoRecordBase(HeapUndoRecordBase &&other) = default;
    HeapUndoRecordBase &operator=(HeapUndoRecordBase &&other) = default;
    
    virtual ~HeapUndoRecordBase() = 0;
};
} // namespace mi::access::heap::undo