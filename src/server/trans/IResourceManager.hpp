#pragma once

#include "storage/undo/IRMgrUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/wal/IRMgrWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include <memory>

namespace mi::storage::trans {

class IResourceManager {
  public:
    // Parse UndoRecord specific for given resource manager read from undo
    virtual std::unique_ptr<undo::IRMgrUndoRecord> ParseUndo(uint8_t type, std::byte *buffer,
                                                   size_t length) = 0;
    // Apply given UNDO record
    virtual void ApplyUndo(undo::IRMgrUndoRecord &record, undo::UndoSeqNumber usn) = 0;
    // Apply given REDO record
    virtual void ApplyRedo(wal::IRMgrWalRecord &record, wal::LogSeqNumber lsn) = 0;

    virtual ~IResourceManager() = default;
};

} // namespace mi::transam
