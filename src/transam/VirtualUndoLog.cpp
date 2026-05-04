#include "mimidb.hpp"

#include "transam/UndoSeqNumber.hpp"
#include "transam/VirtualUndoLog.hpp"

#include "cluster_state.hpp"
#include <stdexcept>

mi::transam::UndoSeqNumber mi::transam::VirtualUndoLog::InsertRecord(IUndoRecord &record) {
    auto usn = UndoLogGlobal->InsertUndoRecord(record);
    this->_history.push_back(usn);
    return usn;
}

void mi::transam::VirtualUndoLog::UndoAllRecords() {
    throw std::runtime_error("not implemented");
}
