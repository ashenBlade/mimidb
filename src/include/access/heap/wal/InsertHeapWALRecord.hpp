#pragma once

#include <cstddef>
#include <cstring>
#include <memory>

#include "access/heap/TupleId.hpp"
#include "access/heap/wal/HeapWALRecord.hpp"


namespace mi::access::heap::wal {
class InsertHeapWALRecord : public HeapWALRecord {
  public:
    /// Identification of tuple location
    TupleId TupId;
    /// Actual tuple bytes
    std::vector<std::byte> TupleData;

    InsertHeapWALRecord(TupleId id, std::vector<std::byte> tuple)
        : HeapWALRecord(HeapWALRecordType::Insert), TupId(id), TupleData(std::move(tuple)) {};

    size_t CalculateSize() const override { return sizeof(TupleId) + this->TupleData.size(); }

    void Serialize(std::byte *buffer) const override {
        auto cursor = buffer;

        // TupleId
        *reinterpret_cast<TupleId *>(cursor) = this->TupId;
        cursor += sizeof(TupleId);

        // Tuple data
        std::memcpy(cursor, this->TupleData.data(), this->TupleData.size());
    }

    ~InsertHeapWALRecord() override = default;
};
} // namespace mi::access::heap::wal
