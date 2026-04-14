#include "mimidb.hpp"

#include "executor/plan/SeqScan.hpp"

namespace mi::executor::plan {

SeqScan::SeqScan(mi::access::table::ITable &table, std::shared_ptr<mi::transam::Snapshot> snapshot)
    : _scan(nullptr), _table(table), _snapshot(snapshot) {};

void SeqScan::Start() {
    auto scan = _table.StartScan(_snapshot);
    scan->BeginScan();
}

void SeqScan::End() {
    _scan->EndScan();
}

std::unique_ptr<mi::access::table::ITuple> SeqScan::Execute() {
    return _scan->GetNextTuple();
}

// No special logic required
SeqScan::~SeqScan() = default;

}; // namespace mi::executor::plan