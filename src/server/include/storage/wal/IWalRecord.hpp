#pragma once

#include "trans/ResourceManagerId.hpp"
#include <cstddef>

namespace mi::storage::wal {

// Interface for representing custom WAL record
class IWalRecord {
  public:
    // Get Resource Manager Id for this record
    virtual trans::ResourceManagerId GetRMgrId() const = 0;
    // Get type of record specific for resource manager
    virtual uint8_t GetType() const = 0;
    // Get size of serialized record
    virtual size_t CalculateSize() const = 0;
    // Serialize record and store it into passed buffer.
    // Size of buffer is at least what CalculateSize() returns.
    virtual void Serialize(std::byte *buffer) const = 0;
    virtual ~IWalRecord() = default;
};

} // namespace mi::transam
