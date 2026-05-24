#pragma once

#include "storage/undo/IRMgrUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/wal/IRMgrWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "trans/IResourceManager.hpp"

#include <memory>

namespace mi::access::heap {

class HeapResourceManager : public storage::trans::IResourceManager {
  public:
    std::unique_ptr<storage::undo::IRMgrUndoRecord> ParseUndo(uint8_t type, std::byte *data, size_t length) override;
    void ApplyUndo(storage::undo::IRMgrUndoRecord &record, storage::undo::UndoSeqNumber usn) override;
    void ApplyRedo(storage::wal::IRMgrWalRecord &record, storage::wal::LogSeqNumber lsn) override;

    ~HeapResourceManager() override = default;

    static HeapResourceManager *Create();
};

} // namespace mi::access::heap
