#include "storage/undo/VirtualUndoLog.hpp"
#include "cluster_state.hpp"

using namespace mi::storage::undo;

UndoSeqNumber VirtualUndoLog::InsertRecord(IRMgrUndoRecord &record) {
    auto usn = UndoLogGlobal->InsertUndoRecord(record);
    this->_history.push_back(usn);
    return usn;
}

void VirtualUndoLog::UndoAllRecords() {
    // Perform undo it backward sequence
    for (auto it = this->_history.rbegin(); it != this->_history.rend(); ++it) {
        UndoLogGlobal->PerformUndo(it->value);
    }

    // Do not forget to clean history
    this->_history.clear();
}
