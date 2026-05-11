#pragma once

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/table/Oid.hpp"
#include "access/heap/TupleId.hpp"

#include <vector>

namespace mi::access::heap::undo {
class InsertUndoRecord : public HeapUndoRecord {
  public:
    // Table Id
    Oid TableId;
    // Tuple location
    TupleId Location;
    // Saved tuple data
    std::vector<std::byte> TupleData;

    InsertUndoRecord(Oid tableId, TupleId location, std::vector<std::byte> tupleData);

    size_t CalculateSize() const override;
    void Serialize(std::byte *buffer) override;

    void Accept(IHeapUndoRecordVisitor &visitor) override;
};
} // namespace mi::access::heap::undo
