#include "executor/plan/UpdateNode.hpp"
#include "access/table/ITuple.hpp"
#include "executor/VirtualTuple.hpp"
#include "trans/Snapshot.hpp"
#include <stdexcept>

using namespace mi::executor::plan;

UpdateNode::UpdateNode(
    access::table::ITable *table, std::unique_ptr<IExpressionNode> qual,
    std::vector<std::pair<access::table::AttrNumber, std::unique_ptr<IExpressionNode>>> updates)
    : _table(table), _qual(std::move(qual)), _updates(std::move(updates)), _scan(nullptr) {
    if (this->_qual == nullptr) {
        throw std::runtime_error("qual is null");
    }
}

void UpdateNode::Start(mi::storage::trans::Snapshot *snapshot) {
    assert(this->_scan == nullptr);

    auto scan = this->_table->StartScan(snapshot);
    scan->BeginScan();
    this->_scan = std::move(scan);
}

void UpdateNode::End() {
    if (this->_scan) {
        this->_scan->EndScan();
        this->_scan = nullptr;
    }
}

std::unique_ptr<mi::access::table::ITuple> UpdateNode::Execute() {
    if (!this->_scan) {
        throw std::runtime_error("scan is not started");
    }

    while (auto oldTuple = this->_scan->GetNextTuple()) {
        // Check old tuple passes predicate
        if (!this->_qual->ExecQual(*oldTuple)) {
            continue;
        }

        // Make new tuple
        auto newTuple = VirtualTuple::Copy(*oldTuple);
        for (auto &[attno, expr] : this->_updates) {
            // Note: pass old tuple, because expression must be evaluated on old values
            auto newVal = expr->Exec(*oldTuple);
            newTuple.SetAttribute(attno, newVal);
        }

        // Update table tuple
        this->_table->UpdateTuple(*oldTuple, newTuple);
    }

    return nullptr;
}
