#pragma once

#include "transam/IResourceManager.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/LogSeqNumber.hpp"
#include "transam/UndoSeqNumber.hpp"
namespace mi::access::heap {

class HeapResourceManager : public transam::IResourceManager {
  public:
    void ApplyUndo(transam::IUndoRecord &record, transam::UndoSeqNumber usn) override;
    void ApplyRedo(transam::IWalRecord &record, transam::LogSeqNumber lsn) override;

    ~HeapResourceManager() = default;

    static HeapResourceManager *Create();
};

} // namespace mi::access::heap
