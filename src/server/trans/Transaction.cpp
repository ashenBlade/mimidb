#include "trans/Transaction.hpp"
#include "cluster_state.hpp"

using namespace mi::storage::trans;

void Transaction::BeginNewStatement() {
    auto csn = TransactionManagerGlobal->GetCurrentCSN();
    auto snapshot = std::make_unique<Snapshot>(csn);

    this->_snapshot = std::move(snapshot);
}
