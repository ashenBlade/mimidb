#include "mimidb.hpp"

#include "lock/Spin.hpp"
#include "transam/CommitSeqNumber.hpp"
#include "transam/Transaction.hpp"
#include "transam/TransactionId.hpp"
#include "worker_state.hpp"
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "transam/TransactionManager.hpp"

using namespace mi::transam;

TransactionManager::TransactionManager() : _csn(CommitSeqNumber::Min), _xid(TransactionId::Min) {}

CommitSeqNumber TransactionManager::GetCurrentCSN() const { return this->_csn.load(); }

TransactionId TransactionManager::GetCurrentXID() const { return this->_xid.load(); }

CommitSeqNumber TransactionManager::GetTransactionCsn(TransactionId xid) {
    do {
        auto lock = std::shared_lock{this->_mutex};
        auto csn = this->_status[xid];
        if (!csn.IsCommitting()) {
            return csn;
        }

        // do not forget to release lock before sleeping
        lock.release();
        mi::lock::Spin::PerformSpin();
    } while (true);
}

TransactionId TransactionManager::BeginNewTransaction() {
    auto xid = std::atomic_fetch_add(&this->_xid, 1);

    // Update global status of this transaction
    auto lock = std::lock_guard{this->_mutex};
    this->_status[xid] = CommitSeqNumber::InProgress;
    return xid;
}

CommitSeqNumber TransactionManager::CommitTransaction(TransactionId xid) {
    // First mark transaction as committing
    WITH(auto lock = std::lock_guard{this->_mutex}) {
        this->_status[xid] = CommitSeqNumber::Committing;
    }

    // Then obtain it's CSN and mark as committed
    auto csn = std::atomic_fetch_add(&this->_csn, 1);

    WITH(auto lock = std::lock_guard{this->_mutex}) {
        this->_status[xid] = csn;
    }

    return csn;
}

void TransactionManager::AbortTransaction(TransactionId xid) {
    auto lock = std::lock_guard{this->_mutex};
    auto &status = this->_status[xid];
    assert(status.IsInProgress());
    status = CommitSeqNumber::Aborted;
}

