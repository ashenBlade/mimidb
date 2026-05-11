#include "cluster_state.hpp"
#include "mimidb.hpp"

#include "transam/Transaction.hpp"

using namespace mi::transam;

void Transaction::BeginNewStatement() {
    auto csn = TransactionManagerGlobal->GetCurrentCSN();
    auto snapshot = std::make_unique<Snapshot>(csn);

    this->_snapshot = std::move(snapshot);
}
 
