#include "mimidb.hpp"

#include "access/heap/undo/InsertUndoRecord.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/undo/IHeapUndoRecordVisitor.hpp"

#include <cstring>

using namespace mi::access::heap::undo;

InsertUndoRecord::InsertUndoRecord(Oid tableId, TupleId location, std::vector<std::byte> tupleData) : HeapUndoRecord(HeapUndoRecordType::Insert),
    TableId(tableId), Location(location), TupleData(std::move(tupleData)) {};

size_t InsertUndoRecord::CalculateSize() const {
    return sizeof(this->TableId) + sizeof(this->Location) + this->TupleData.size();
}

void InsertUndoRecord::Serialize(std::byte *buffer) {
    auto cursor = buffer;

    *reinterpret_cast<Oid *>(cursor) = this->TableId;
    cursor += sizeof(Oid);

    *reinterpret_cast<TupleId *>(cursor) = this->Location;
    cursor += sizeof(TupleId);

    std::memcpy(cursor, this->TupleData.data(), this->TupleData.size());
}

void InsertUndoRecord::Accept(IHeapUndoRecordVisitor &visitor) {
    visitor.Visit(*this);
}
