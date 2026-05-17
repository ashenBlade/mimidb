#include "executor/plan/DeleteNode.hpp"
#include "access/table/ITuple.hpp"
#include "trans/Snapshot.hpp"
#include <memory>
#include <stdexcept>

using namespace mi::executor::plan;

DeleteNode::DeleteNode(access::table::ITable *table, std::unique_ptr<IExpressionNode> qual)
    : _table(table), _qual(std::move(qual)), _scan(nullptr) {
    if (this->_qual == nullptr) {
        throw std::runtime_error("predicate is not provided");
    }
};

void DeleteNode::Start(mi::storage::trans::Snapshot *snapshot) {
    // nothing
    auto scan = this->_table->StartScan(snapshot);
    scan->BeginScan();
    this->_scan = std::move(scan);
}

void DeleteNode::End() {
    if (this->_scan) {
        this->_scan->EndScan();
        this->_scan = nullptr;
    }
}

std::unique_ptr<mi::access::table::ITuple> DeleteNode::Execute() {
    if (!this->_scan) {
        throw std::runtime_error("Start was not invoked");
    }

    while (auto tuple = this->_scan->GetNextTuple()) {
        if (!this->_qual->ExecQual(*tuple)) {
            continue;
        }

        this->_table->DeleteTuple(*tuple);
    }

    return nullptr;
}
