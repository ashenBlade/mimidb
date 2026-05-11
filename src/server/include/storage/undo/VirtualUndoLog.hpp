#pragma once

#include "storage/undo/IRMgrUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "utils/NonCopyable.hpp"
#include <vector>

namespace mi::storage::undo {
class VirtualUndoLog : public NonCopyable {
  private:
    // Locations of all inserted records
    std::vector<UndoSeqNumber> _history;

  public:
    // Insert new record into global undo log
    UndoSeqNumber InsertRecord(IRMgrUndoRecord &record);
    // Perform undo of all written entries for this virtual log
    void UndoAllRecords();
};
} // namespace mi::transam
