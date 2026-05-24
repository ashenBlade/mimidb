#include "trans/Transaction.hpp"
#include "cluster_state.hpp"
#include "lock/LockMode.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "trans/CommitSeqNumber.hpp"
#include "worker_state.hpp"
#include <memory>
#include <shared_mutex>

using namespace mi::storage::trans;

void Transaction::BeginNewStatement() {
    // Creates new snapshot and increment CID if required

    if (this->_snapshot == nullptr) {
        auto csn = TransactionManagerGlobal->GetCurrentCSN();
        this->_snapshot = std::make_unique<Snapshot>(csn, undo::UndoSeqNumber::Invalid);
    } else if (this->_undoLog != nullptr) {
        auto last = this->_undoLog->LastUSN();
        if (last.IsValid()) {
            this->_snapshot = std::make_unique<Snapshot>(this->_snapshot->CSN(), last + 1);
        }
    } else {
        // snapshot CID did not change, since there were no DML (undo log does not exist)
    }

    assert(this->_snapshot != nullptr);
}

void Transaction::Wait() {
    assert(this->_xid != MyTransaction->GetXID());
    // Owner worker holds X lock, so we will get S lock only when transaction ends
    // NOTE: we need to have unlock it, otherwise it will be left in locked state
    auto lock = std::shared_lock{this->_latch};
}

void Transaction::Commit(CommitSeqNumber csn) {
    assert(csn.IsNormal());

    this->_csn = csn;
    this->_status = TransactionStatus::COMMITTED;

    this->_latch.Unlock(lock::LockMode::Exclusive);
}

void Transaction::Abort() {
    this->_status = TransactionStatus::ABORTED;
    this->_latch.Unlock(lock::LockMode::Exclusive);
}

mi::storage::undo::VirtualUndoLog &Transaction::GetUndoLog() {
    if (this->_undoLog == nullptr) {
        this->_undoLog = std::make_unique<undo::VirtualUndoLog>();
    }
    return *this->_undoLog.get();
}
