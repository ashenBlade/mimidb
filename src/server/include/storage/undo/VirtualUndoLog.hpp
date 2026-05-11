#pragma once

#include <vector>

#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/undo/IUndoRecord.hpp"
#include "utils/NonCopyable.hpp"

namespace mi::transam {
class VirtualUndoLog : public NonCopyable {
  private:
    // Locations of all inserted records
    std::vector<UndoSeqNumber> _history;

  public:
    // Insert new record into global undo log
    UndoSeqNumber InsertRecord(IUndoRecord &record);
    // Perform undo of all written entries for this virtual log
    void UndoAllRecords();
};
} // namespace mi::transam
