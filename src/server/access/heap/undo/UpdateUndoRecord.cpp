#include "mimidb.hpp"

#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/table/Oid.hpp"
#include "access/heap/undo/IHeapUndoRecordVisitor.hpp"

#include <cstring>

using namespace mi::access::heap::undo;

UpdateUndoRecord::UpdateUndoRecord(Oid tableId, TupleId oldLocation, TupleId newLocation,
                                   std::vector<std::byte> tupleData)
    : HeapUndoRecord(HeapUndoRecordType::Update), TableId(tableId), OldLocation(oldLocation),
      NewLocation(newLocation), TupleData(std::move(tupleData)) {}

size_t UpdateUndoRecord::CalculateSize() const {
    return sizeof(this->TableId) + sizeof(this->OldLocation) + sizeof(this->NewLocation) + this->TupleData.size();
}

void UpdateUndoRecord::Serialize(std::byte *buffer) {
    auto cursor = buffer;

    *reinterpret_cast<Oid *>(cursor) = this->TableId;
    cursor += sizeof(Oid);

    *reinterpret_cast<TupleId *>(cursor) = this->OldLocation;
    cursor += sizeof(TupleId);

    *reinterpret_cast<TupleId *>(cursor) = this->NewLocation;
    cursor += sizeof(TupleId);

    std::memcpy(cursor, this->TupleData.data(), this->TupleData.size());
}

void UpdateUndoRecord::Accept(IHeapUndoRecordVisitor &visitor) {
    visitor.Visit(*this);
}
