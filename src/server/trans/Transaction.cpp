#include "trans/Transaction.hpp"
#include "cluster_state.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include <memory>

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
