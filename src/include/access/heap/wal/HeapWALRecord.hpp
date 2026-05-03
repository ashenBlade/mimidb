#pragma once

#include "transam/IWalRecord.hpp"
#include "transam/ResourceManagerId.hpp"
#include <cstdint>

namespace mi::access::heap::wal {
enum HeapWALRecordType : uint8_t { Insert = 1, Update = 2, Delete = 3 };

// Base class for HEAP WAL records
template <HeapWALRecordType VType> class HeapWALRecord : public transam::IWalRecord {
  public:
    transam::ResourceManagerId GetRMgrId() const override {
        return transam::ResourceManagerId::Heap;
    }
    uint8_t GetType() const override { return VType; }
};
} // namespace mi::access::heap::wal
