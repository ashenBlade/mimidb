#pragma once

#include "transam/Snapshot.hpp"
#include "transam/TransactionId.hpp"
#include "transam/VirtualUndoLog.hpp"
#include <memory>
#include <optional>

namespace mi::transam {
enum class TransactionStatus {
    RUNNING = 1,   // Executing statement
    COMMITTED = 2, // Successfully comitted
    ABORTED = 3,   // Aborted
};

class Transaction {
  private:
    /// @brief Assigned XID for this transaction
    TransactionId _xid;
    /// @brief Snapshot for this transaction
    Snapshot _snapshot;
    /// @brief Status of current transaction
    TransactionStatus _status;
    /// @brief Undo Log for this transaction
    std::unique_ptr<VirtualUndoLog> _undoLog;

  public:
    Transaction(TransactionId xid, Snapshot snapshot)
        : _xid(xid), _snapshot(snapshot), _status(TransactionStatus::RUNNING) {}
    TransactionId GetXID() const { return this->_xid; }
    Snapshot *GetSnapshot() { return &this->_snapshot; }
    const Snapshot *GetSnapshot() const { return &this->_snapshot; }

    TransactionStatus GetStatus() const { return this->_status; }
    void SetStatus(TransactionStatus status) { this->_status = status; }

    VirtualUndoLog &GetUndoLog() {
        if (this->_undoLog == nullptr) {
            this->_undoLog = std::make_unique<VirtualUndoLog>();
        }
        return *this->_undoLog.get();
    }

    VirtualUndoLog *GetUndoLogIfAny() const { return this->_undoLog.get(); }
};

}; // namespace mi::transam