#pragma once

#include "transam/IUndoRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/LogSeqNumber.hpp"
#include "transam/UndoSeqNumber.hpp"

namespace mi::transam {

class IResourceManager {
    public:
        // Apply given UNDO record
        virtual void ApplyUndo(IUndoRecord &record, UndoSeqNumber usn) = 0;
        // Apply given REDO record
        virtual void ApplyRedo(IWalRecord &record, LogSeqNumber lsn) = 0;

        virtual ~IResourceManager() = default;
};

}
