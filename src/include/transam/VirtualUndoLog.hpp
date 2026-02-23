#pragma once

#include <memory>

#include "transam/TransactionId.hpp"
#include "transam/UndoSeqNumber.hpp"

namespace mi::transam {

// Forward declaration, due to cycle dependency with UndoLog
class UndoLog;

class VirtualUndoLog {
  private:
    /// @brief Undo log to which this virtual log belongs
    std::shared_ptr<UndoLog> _log;
    /// @brief Transaction by which tnx log is used
    TransactionId _xid;
  public:
    VirtualUndoLog(std::shared_ptr<UndoLog> log, TransactionId xid);

    /// @brief Durable write undo record to log
    /// @param record Record to write
    /// @return Address of undo log record to start of record
    UndoSeqNumber WriteUndoRecord();
};

};