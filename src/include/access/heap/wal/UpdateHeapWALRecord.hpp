#pragma once

#include "access/heap/TupleId.hpp"
#include "access/heap/wal/HeapWALRecord.hpp"
#include "access/table/Oid.hpp"
#include <cstring>

namespace mi::access::heap::wal {
class UpdateHeapWALRecord : public HeapWALRecord {
  public:
    // Table Id
    Oid TableId;
    // Location of old tuple
    TupleId OldTupleId;
    // Location of new tuple
    TupleId NewTupleId;
    // New tuple data
    std::vector<std::byte> TupleData;
    UpdateHeapWALRecord(Oid tableId, TupleId oldTupleId, TupleId newTupleId,
                        std::vector<std::byte> tupleData)
        : HeapWALRecord(HeapWALRecordType::Update), TableId(tableId), OldTupleId(oldTupleId),
          NewTupleId(newTupleId), TupleData(std::move(tupleData)) {};
    size_t CalculateSize() const override {
        return sizeof(this->TableId) + sizeof(this->OldTupleId) + sizeof(this->NewTupleId) +
               this->TupleData.size();
    }

    void Serialize(std::byte *buffer) const override {
        auto cursor = buffer;

        *reinterpret_cast<Oid *>(cursor) = this->TableId;
        cursor += sizeof(Oid);

        *reinterpret_cast<TupleId *>(cursor) = this->OldTupleId;
        cursor += sizeof(TupleId);

        *reinterpret_cast<TupleId *>(cursor) = this->NewTupleId;
        cursor += sizeof(TupleId);

        std::memcpy(cursor, this->TupleData.data(), this->TupleData.size());
    }

    ~UpdateHeapWALRecord() override = default;
};
} // namespace mi::access::heap::wal
