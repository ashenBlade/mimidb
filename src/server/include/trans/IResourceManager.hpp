#pragma once

#include "storage/undo/IUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/wal/IWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include <memory>

namespace mi::transam {

class IResourceManager {
    public:
        // Parse UndoRecord specific for given resource manager read from undo
        virtual std::unique_ptr<IUndoRecord> ParseUndo(uint8_t type, std::byte *buffer, size_t length) = 0;
        // Apply given UNDO record
        virtual void ApplyUndo(IUndoRecord &record, UndoSeqNumber usn) = 0;
        // Apply given REDO record
        virtual void ApplyRedo(IWalRecord &record, LogSeqNumber lsn) = 0;

        virtual ~IResourceManager() = default;
};

}
