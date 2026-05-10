#pragma once

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/ResourceManagerId.hpp"

namespace mi::access::heap::undo {
enum HeapUndoRecordType : uint8_t {
    Delete = 1,
    Update = 2,
};

// Forward declaration for visitor pattern
class IHeapUndoRecordVisitor;

// Base class for all heap undo records
class HeapUndoRecord : public transam::IUndoRecord {
  protected:
    HeapUndoRecordType _type;

  public:
    HeapUndoRecord(HeapUndoRecordType type): _type(type) {};

    HeapUndoRecord(const HeapUndoRecord &other) = default;
    HeapUndoRecord &operator=(const HeapUndoRecord &other) = default;
    HeapUndoRecord(HeapUndoRecord &&other) = default;
    HeapUndoRecord &operator=(HeapUndoRecord &&other) = default;

    transam::ResourceManagerId GetRMgrId() const override {
        return transam::ResourceManagerId::Heap;
    }

    uint8_t GetType() const override { return this->_type; }

    virtual void Accept(IHeapUndoRecordVisitor &visitor) = 0;

    virtual ~HeapUndoRecord() = default;
};
} // namespace mi::access::heap::undo