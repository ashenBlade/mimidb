#pragma once

#include "trans/ResourceManagerId.hpp"
#include <cstddef>

namespace mi::transam {

class IUndoRecord {
  public:
    // Get resource manager for this record
    virtual ResourceManagerId GetRMgrId() const = 0;
    // Get type of record specific for resource manager
    virtual uint8_t GetType() const = 0;
    // Calculate all size required for this record
    virtual size_t CalculateSize() const = 0;
    // Serialize record and store it into passed buffer.
    // Size of buffer is at least what CalculateSize() returns.
    virtual void Serialize(std::byte *buffer) = 0;

    virtual ~IUndoRecord() = default;
};

} // namespace mi::transam
