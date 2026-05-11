#pragma once

#include "access/heap/TupleId.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/table/Oid.hpp"

namespace mi::access::heap::undo {
class DeleteUndoRecord : public HeapUndoRecord {
  public:
    // Identifier of tuple to delete
    Oid TableId;
    TupleId TupId;
    DeleteUndoRecord(Oid tableId, TupleId tupId) : HeapUndoRecord(HeapUndoRecordType::Delete), TableId(tableId), TupId(tupId) {};

    // Serialized record size
    static constexpr const size_t Size = sizeof(TableId) + sizeof(TupId);

    size_t CalculateSize() const override { return DeleteUndoRecord::Size; }

    void Serialize(std::byte *buffer) override {
        auto cursor = buffer;

        *reinterpret_cast<Oid *>(cursor) = this->TableId;
        cursor += sizeof(Oid);

        *reinterpret_cast<TupleId *>(cursor) = this->TupId;
    }

    void Accept(IHeapUndoRecordVisitor &visitor) override;
};
} // namespace mi::access::heap::undo
