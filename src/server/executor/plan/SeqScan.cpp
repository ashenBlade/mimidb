#include "executor/plan/SeqScan.hpp"

namespace mi::executor::plan {

SeqScan::SeqScan(mi::access::table::ITable *table, mi::storage::trans::Snapshot *snapshot,
                 std::unique_ptr<IExpressionNode> qual)
    : _scan(nullptr), _table(table), _snapshot(snapshot), _qual(std::move(qual)) {};

void SeqScan::Start() {
    auto scan = _table->StartScan(this->_snapshot);
    scan->BeginScan();
    this->_scan = std::move(scan);
}

void SeqScan::End() { _scan->EndScan(); }

std::unique_ptr<mi::access::table::ITuple> SeqScan::Execute() {
    while (auto tuple = _scan->GetNextTuple()) {
        if (this->_qual) {
            auto result = this->_qual->Exec(*tuple);
            if (result.has_value() && result.value().getScalar<bool>()) {
                return tuple;
            }
        } else {
            return tuple;
        }
    }

    return nullptr;
}

}; // namespace mi::executor::plan