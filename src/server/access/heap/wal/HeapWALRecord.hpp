#pragma once

#include "storage/wal/IRMgrWalRecord.hpp"
#include "trans/ResourceManagerId.hpp"
#include <cstdint>

namespace mi::access::heap::wal {
enum HeapWALRecordType : uint8_t {
    Insert = 1,
    Update = 2,
    Delete = 3,
};

// Base class for HEAP WAL records
class HeapWALRecord : public storage::wal::IRMgrWalRecord {
  protected:
    HeapWALRecordType _type;

  public:
    HeapWALRecord(HeapWALRecordType type) : _type(type) {};
    storage::trans::ResourceManagerId GetRMgrId() const override {
        return storage::trans::ResourceManagerId::Heap;
    };
    uint8_t GetType() const override { return this->_type; };
};
} // namespace mi::access::heap::wal
