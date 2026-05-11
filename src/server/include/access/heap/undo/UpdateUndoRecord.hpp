#pragma once

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/TupleId.hpp"
#include "access/table/Oid.hpp"

#include <vector>

namespace mi::access::heap::undo {

class UpdateUndoRecord : public HeapUndoRecord {
  public:
    // Updated table id
    Oid TableId;
    // Old tuple location
    TupleId OldLocation;
    // New tuple location
    TupleId NewLocation;
    // Saved tuple data (which was updated)
    std::vector<std::byte> TupleData;

    UpdateUndoRecord(Oid tableId, TupleId oldLocation, TupleId newLocation, std::vector<std::byte> tupleData);

    size_t CalculateSize() const override;
    void Serialize(std::byte *buffer) override;

    void Accept(IHeapUndoRecordVisitor &visitor) override;
};
} // namespace mi::access::heap::undo
