#pragma once

#include "transam/IWalRecord.hpp"
#include "transam/ResourceManagerId.hpp"
#include <cstdint>

namespace mi::access::heap::wal {
enum HeapWALRecordType : uint8_t { Insert = 1 };

// Base class for HEAP WAL records
class HeapWALRecord : public transam::IWalRecord {
  protected:
    HeapWALRecordType _type;

  public:
    HeapWALRecord(HeapWALRecordType type) : _type(type) {};
    transam::ResourceManagerId GetRMgrId() const override {
        return transam::ResourceManagerId::Heap;
    };
    uint8_t GetType() const override { return this->_type; };
};
} // namespace mi::access::heap::wal
