#include "trans/TransactionManager.hpp"
#include "lock/Spin.hpp"
#include "mimidb.hpp"
#include "trans/CommitSeqNumber.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionId.hpp"
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

using namespace mi::storage::trans;

TransactionManager::TransactionManager() : _csn(CommitSeqNumber::Min), _xid(TransactionId::Min) {}

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

    auto lock = std::lock_guard{this->_mutex};
    // Create transaction object only in the lock, because otherwise there may be race condition
    auto transaction = std::make_unique<Transaction>(xid);
    auto ptr = transaction.get();
    this->_state[xid] = std::move(transaction);
    return ptr;
}

CommitSeqNumber TransactionManager::CommitTransaction(TransactionId xid) {
    // First mark transaction as committing
    WITH(auto lock = std::lock_guard{this->_mutex}) {
        this->_history[xid] = CommitSeqNumber::Committing;
    }

    // Then obtain it's CSN and mark as committed
    auto csn = std::atomic_fetch_add(&this->_csn, 1);

    WITH(auto lock = std::lock_guard{this->_mutex}) {
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
    auto lock = std::lock_guard{this->_mutex};

    auto &status = this->_history[xid];
    assert(status.IsInProgress());
    status = CommitSeqNumber::Aborted;

    auto it = this->_state.find(xid);
    if (it == this->_state.end()) {
        throw std::runtime_error("Transaction table is broken - no transaction entry found");
    }

    this->_state.erase(it);
}

void TransactionManager::WaitTransactionEnd([[maybe_unused]] TransactionId xid) {
    throw std::runtime_error("WaitTransactionEnd is not implemented");
}
