#pragma once

#include "transam/IResourceManager.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/LogSeqNumber.hpp"
#include "transam/UndoSeqNumber.hpp"
#include <memory>
namespace mi::access::heap {

class HeapResourceManager : public transam::IResourceManager {
  public:
    std::unique_ptr<transam::IUndoRecord> ParseUndo(uint8_t type, std::byte *data, size_t length) override;
    void ApplyUndo(transam::IUndoRecord &record, transam::UndoSeqNumber usn) override;
    void ApplyRedo(transam::IWalRecord &record, transam::LogSeqNumber lsn) override;

    ~HeapResourceManager() override = default;

    static HeapResourceManager *Create();
};

} // namespace mi::access::heap
