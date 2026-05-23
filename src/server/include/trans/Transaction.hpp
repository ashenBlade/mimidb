#pragma once

#include "lock/LWLatch.hpp"
#include "lock/LockMode.hpp"
#include "storage/undo/VirtualUndoLog.hpp"
#include "trans/Snapshot.hpp"
#include "trans/TransactionId.hpp"
#include "worker/WorkerId.hpp"
#include <cassert>
#include <memory>

namespace mi::storage::trans {
enum class TransactionStatus {
    RUNNING = 1,   // Executing statement
    COMMITTED = 2, // Successfully comitted
    ABORTED = 3,   // Aborted
};

class Transaction {
  private:
    /// @brief Assigned XID for this transaction
    TransactionId _xid;
    /// @brief Latch to await transaction end
    lock::LWLatch _latch;
    /// @brief Snapshot for current statement
    std::unique_ptr<Snapshot> _snapshot;
    /// @brief Status of current transaction
    TransactionStatus _status;
    /// @brief Undo Log for this transaction
    std::unique_ptr<undo::VirtualUndoLog> _undoLog;
    /// @brief Worker owning transaction
    worker::WorkerId _workerId;

  public:
    Transaction(TransactionId xid, worker::WorkerId workerId)
        : _xid(xid), _latch(), _snapshot(nullptr), _status(TransactionStatus::RUNNING),
          _undoLog(nullptr), _workerId(workerId) {
        // Creator of transaction owns is a worker, that runs this transaction
        _latch.Lock(lock::LockMode::Exclusive);
    }

    // Begin execution of new statement in transaction.
    // For now only new snapshot is being established.
    void BeginNewStatement();

    TransactionId GetXID() const { return this->_xid; }

    Snapshot *GetSnapshot() { return this->_snapshot.get(); }
    const Snapshot *GetSnapshot() const { return this->_snapshot.get(); }

    TransactionStatus GetStatus() const { return this->_status; }
    void SetStatus(TransactionStatus status) { this->_status = status; }

    undo::VirtualUndoLog *GetUndoLogIfAny() const { return this->_undoLog.get(); }
    worker::WorkerId GetWorkerId() const { return this->_workerId; }
    
    undo::VirtualUndoLog &GetUndoLog();
    void Wait();
    ~Transaction();
};

}; // namespace mi::storage::trans