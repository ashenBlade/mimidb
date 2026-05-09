#include "mimidb.hpp"

#include "transam/UndoSeqNumber.hpp"
#include "transam/VirtualUndoLog.hpp"

#include "cluster_state.hpp"

mi::transam::UndoSeqNumber mi::transam::VirtualUndoLog::InsertRecord(IUndoRecord &record) {
    auto usn = UndoLogGlobal->InsertUndoRecord(record);
    this->_history.push_back(usn);
    return usn;
}

void mi::transam::VirtualUndoLog::UndoAllRecords() {
    // Perform undo it backward sequence
    for (auto it = this->_history.rbegin(); it != this->_history.rend(); ++it) {
        UndoLogGlobal->PerformUndo(it->value);
    }

    // Do not forget to clean history
    this->_history.clear();
}
