#pragma once

#include "storage/undo/IUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/wal/IWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "trans/IResourceManager.hpp"

#include <memory>

namespace mi::access::heap {

class HeapResourceManager : public storage::trans::IResourceManager {
  public:
    std::unique_ptr<storage::undo::IUndoRecord> ParseUndo(uint8_t type, std::byte *data, size_t length) override;
    void ApplyUndo(storage::undo::IUndoRecord &record, storage::undo::UndoSeqNumber usn) override;
    void ApplyRedo(storage::wal::IWalRecord &record, storage::wal::LogSeqNumber lsn) override;

    ~HeapResourceManager() override = default;

    static HeapResourceManager *Create();
};

} // namespace mi::access::heap
