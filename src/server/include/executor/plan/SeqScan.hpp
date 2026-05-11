#pragma once

#include "access/table/ITable.hpp"
#include "access/table/ITableScan.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>

namespace mi::executor::plan {
class SeqScan : public IPlanNode {
  private:
    /// @brief State for scanning node
    std::unique_ptr<mi::access::table::ITableScan> _scan;
    /// @brief Table we are going to scan
    mi::access::table::ITable &_table;
    /// @brief Snapshot to use during scan
    std::shared_ptr<mi::transam::Snapshot> _snapshot;

  public:
    SeqScan(mi::access::table::ITable &table, std::shared_ptr<mi::transam::Snapshot> snapshot);

    void Start() override;
    void End() override;
    std::unique_ptr<mi::access::table::ITuple> Execute() override;
    ~SeqScan() override;
};
}; // namespace mi::executor::plan