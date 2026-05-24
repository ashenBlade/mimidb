#include "trans/TransactionManager.hpp"
#include "lock/Spin.hpp"
#include "trans/CommitSeqNumber.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionId.hpp"
#include "worker/WorkerId.hpp"
#include "worker_state.hpp"
#include <assert.h>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

using namespace mi::storage::trans;

TransactionManager::TransactionManager(std::size_t workersCount)
    : _csn(CommitSeqNumber::Min), _xid(TransactionId::Min), _mutex(), _transactions(workersCount),
      _xidToWorker(), _history() {}

CommitSeqNumber TransactionManager::GetCurrentCSN() const { return this->_csn.load(); }

TransactionId TransactionManager::GetCurrentXID() const { return this->_xid.load(); }

CommitSeqNumber TransactionManager::GetTransactionCsn(TransactionId xid) {
    do {
        auto lock = std::shared_lock{this->_mutex};
        auto csn = this->_history[xid];
        if (!csn.IsCommitting()) {
            return csn;
        }

        // do not forget to release lock before sleeping
        lock.release();
        mi::lock::Spin::PerformSpin();
    } while (true);
}

void TransactionManager::BeginNewTransaction() {
    auto xid = std::atomic_fetch_add(&this->_xid, 1);
    auto workerId = MyWorker->GetId();
    auto transaction = std::make_unique<Transaction>(xid, workerId);
    this->_transactions[workerId] = std::move(transaction);
    MyTransaction = this->_transactions[workerId].get();

    // Now update global structures
    auto lock = std::unique_lock{this->_mutex};
    this->_xidToWorker[xid] = workerId;
    this->_history[xid] = CommitSeqNumber::InProgress;
}

void TransactionManager::CommitTransaction() {
    auto workerId = MyWorker->GetId();
    auto &transaction = this->_transactions[workerId];

    // Verify this transaction belongs to us
    assert(transaction != nullptr);
    assert(transaction->GetWorkerId() == MyWorker->GetId());

    auto xid = transaction->GetXID();

    // First mark transaction as committing
    {
        auto lock = std::unique_lock{this->_mutex};
        this->_history[xid] = CommitSeqNumber::Committing;
    }

    // Then obtain it's CSN and mark as committed
    auto csn = std::atomic_fetch_add(&this->_csn, 1);

    {
        // Set it's CSN
        auto lock = std::unique_lock{this->_mutex};
        this->_history[xid] = csn;

        // And remove it's record from map (marking it's ended)
        auto it = this->_xidToWorker.find(xid);
        if (it == this->_xidToWorker.end()) {
            throw std::runtime_error("XidToWorker transaction table is broken - no entry found");
        }

        this->_xidToWorker.erase(it);
    }

    // Mark transaction committed. This will unlocks and release all waiters.
    transaction->Commit(csn);

    // Finally, we can remove transaction object
    this->_transactions[workerId] = nullptr;
    MyTransaction = nullptr;
}

void TransactionManager::AbortTransaction() {
    // Before performing any changes we must undo all changes done, because
    // there is not cooperative garbage collection yet.
    if (auto undoLog = mi::MyTransaction->GetUndoLogIfAny()) {
        undoLog->UndoAllRecords();
    }

    // Now we can perform changes to internal data

    // First mark transaction as aborted in internal table
    auto workerId = MyWorker->GetId();
    auto &transaction = this->_transactions[workerId];

    // Verify this transaction belongs to us
    assert(transaction != nullptr);
    assert(transaction->GetWorkerId() == MyWorker->GetId());

    auto xid = transaction->GetXID();
    {
        auto lock = std::unique_lock{this->_mutex};
        // Mark transaction aborted
        this->_history[xid] = CommitSeqNumber::Aborted;

        // And delete it's worker information
        auto it = this->_xidToWorker.find(xid);
        if (it == this->_xidToWorker.end()) {
            throw std::runtime_error("XidToWorker tnx table is broken - no transaction entry found");
        }

        this->_xidToWorker.erase(it);
    }
    
    transaction->Abort();

    this->_transactions[workerId] = nullptr;
    MyTransaction = nullptr;
}

void TransactionManager::WaitTransactionEnd(TransactionId xid) {
    auto workerId = worker::WorkerId::Invalid();

    {
        auto lock = std::shared_lock{this->_mutex};
        auto it = this->_xidToWorker.find(xid);
        if (it == this->_xidToWorker.end()) {
            // Transaction does not exist.
            // This can happen when transaction ends execution before this function is called.
            return;
        }
    
        workerId = it->second;
    }
    
    // xid2worker must not contain invalid ids
    assert(workerId != worker::WorkerId::Invalid());

    auto transaction = this->_transactions[workerId];

    if (transaction == nullptr) {
        // Target transaction concurrently committed or aborted, so it was deleted from table
        return;
    }

    if (transaction->GetXID() != xid) {
        // Target transaction concurrently committed or aborted and worker started another transaction
        return;
    }

    assert(transaction->GetWorkerId() == workerId);

    // Wait for our transaction
    transaction->Wait();
}
