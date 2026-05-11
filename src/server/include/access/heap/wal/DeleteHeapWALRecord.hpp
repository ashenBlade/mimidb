#pragma once

#include "access/heap/TupleId.hpp"
#include "access/heap/wal/HeapWALRecord.hpp"
#include "executor/Oid.hpp"

namespace mi::access::heap::wal {
class DeleteHeapWALRecord : public HeapWALRecord {
  public:
    Oid TableId;
    TupleId Location;
    DeleteHeapWALRecord(Oid tableId, TupleId location)
        : HeapWALRecord(HeapWALRecordType::Delete), TableId(tableId), Location(location) {};

    size_t CalculateSize() const override { return sizeof(this->TableId) + sizeof(this->Location); }

    void Serialize(std::byte *buffer) const override {
        auto cursor = buffer;

        *reinterpret_cast<Oid *>(cursor) = this->TableId;
        cursor += sizeof(Oid);

        *reinterpret_cast<TupleId *>(cursor) = this->Location;
    }

    ~DeleteHeapWALRecord() override = default;
};
} // namespace mi::access::heap::wal
