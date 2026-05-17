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
    UndoSeqNumber LastUSN() const {
        if (this->_history.size()) {
            return this->_history.back().value;
        } else {
            return UndoSeqNumber::Invalid;
        }
    }
};
} // namespace mi::storage::undo
