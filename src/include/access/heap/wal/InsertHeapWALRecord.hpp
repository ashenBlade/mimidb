#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/TupleId.hpp"
#include "access/heap/wal/HeapWALRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/ResourceManagerId.hpp"
#include <cstddef>
#include <cstring>
#include <memory>

namespace mi::access::heap::wal {
class InsertHeapWALRecord : public HeapWALRecord<HeapWALRecordType::Insert> {
  public:
    /// Identification of tuple locationa
    TupleId TupId;
    /// Actual tuple bytes
    std::shared_ptr<std::vector<std::byte>> Tuple;

    InsertHeapWALRecord(TupleId id, std::shared_ptr<std::vector<std::byte>> tuple)
        : TupId(id), Tuple(std::move(tuple)) {};

    size_t CalculateSize() const override { return sizeof(TupleId) + this->Tuple->size(); }

    void Serialize(std::byte *buffer) const override {
        auto cursor = buffer;

        // TupleId
        *reinterpret_cast<TupleId *>(cursor) = this->TupId;
        cursor += sizeof(TupleId);

        // Tuple data
        std::memcpy(cursor, this->Tuple->data(), this->Tuple->size());
    }

    ~InsertHeapWALRecord() override = default;
};
} // namespace mi::access::heap::wal
