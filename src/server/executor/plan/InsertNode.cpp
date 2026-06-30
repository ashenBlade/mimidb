#include "executor/plan/InsertNode.hpp"
#include "access/ITuple.hpp"
#include "trans/Snapshot.hpp"
#include <memory>

using namespace mi::executor::plan;

InsertNode::InsertNode(access::ITable *table, std::vector<std::unique_ptr<access::ITuple>> tuple)
    : _table(table), _tuples(std::move(tuple)) {}

void InsertNode::Start([[maybe_unused]] mi::storage::trans::Snapshot *snapshot) {
    // nothing to do
}

void InsertNode::End() {
    // nothing to do
}

std::unique_ptr<mi::access::ITuple> InsertNode::Execute() {
    // Insert all tuples
    for (auto &tuple : this->_tuples) {
        this->_table->InsertTuple(*tuple);
        this->_nrows++;
    }

    // We do not return any data
    return nullptr;
}
