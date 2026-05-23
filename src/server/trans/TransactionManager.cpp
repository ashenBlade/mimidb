#include "trans/TransactionManager.hpp"
#include "lock/Spin.hpp"
#include "trans/CommitSeqNumber.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionId.hpp"
#include "worker_state.hpp"
#include <assert.h>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

using namespace mi::storage::trans;

TransactionManager::TransactionManager() : _mutex(), _csn(CommitSeqNumber::Min), _xid(TransactionId::Min) {}

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

Transaction *TransactionManager::BeginNewTransaction() {
    auto xid = std::atomic_fetch_add(&this->_xid, 1);

    auto lock = std::unique_lock{this->_mutex};
    // Create transaction object only in the lock, because otherwise there may be race condition.
    // Note, this will automatically take lock on transaction in X mode.
    auto transaction = std::make_unique<Transaction>(xid, MyWorker->GetId());
    auto ptr = transaction.get();
    this->_state[xid] = std::move(transaction);
    this->_history[xid] = CommitSeqNumber::InProgress;
    return ptr;
}

CommitSeqNumber TransactionManager::CommitTransaction(TransactionId xid) {
    // First mark transaction as committing
    {
        auto lock = std::unique_lock{this->_mutex};
        this->_history[xid] = CommitSeqNumber::Committing;
    }

    // Then obtain it's CSN and mark as committed
    auto csn = std::atomic_fetch_add(&this->_csn, 1);

    {
        auto lock = std::unique_lock{this->_mutex};
        this->_history[xid] = csn;

        // And finally, remove transaction object
        auto it = this->_state.find(xid);
        if (it == this->_state.end()) {
            throw std::runtime_error("Transaction table is broken - no transaction entry found");
        }

        this->_state.erase(it);
    }

    return csn;
}

void TransactionManager::AbortTransaction(TransactionId xid) {
    // Перед тем как помечать транзакцию отменной откатываем все ее изменения.
    // Делаем это перед, т.к. пока нет механизма кооперативной отмены.
    if (auto undoLog = mi::MyTransaction->GetUndoLogIfAny()) {
        undoLog->UndoAllRecords();
    }

    auto lock = std::unique_lock{this->_mutex};

    auto &status = this->_history[xid];
    assert(status.IsInProgress());
    status = CommitSeqNumber::Aborted;

    auto it = this->_state.find(xid);
    if (it == this->_state.end()) {
        throw std::runtime_error("Transaction table is broken - no transaction entry found");
    }

    // Deleting will automatically release lock
    this->_state.erase(it);
}

void TransactionManager::WaitTransactionEnd(TransactionId xid) {
    auto lock = std::shared_lock{this->_mutex};
    auto it = this->_state.find(xid);
    if (it == this->_state.end()) {
        // Transaction does not exist.
        // This can happen when transaction ends execution before this function is called.
        return;
    }

    auto tnx = it->second.get();
    lock.unlock();

    // TODO: вот тут гонка может быть, т.к. после отпускания лока транзакция может быть удалена,
    // поэтому указатель станет невалидным
    tnx->Wait();
}
