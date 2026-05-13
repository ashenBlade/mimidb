#pragma once

#include "access/table/ITable.hpp"
#include "access/table/ITableScan.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>

namespace mi::executor::plan {
class SeqScan : public IPlanNode {
  private:
    /// @brief State for scanning node
    std::unique_ptr<mi::access::table::ITableScan> _scan;
    /// @brief Table we are going to scan
    mi::access::table::ITable *_table;
    /// @brief Snapshot to use during scan
    mi::storage::trans::Snapshot *_snapshot;
    /// @brief Predicate for SELECT
    std::unique_ptr<IExpressionNode> _qual;

  public:
    SeqScan(mi::access::table::ITable *table, mi::storage::trans::Snapshot *snapshot,
            std::unique_ptr<IExpressionNode> qual);

    void Start() override;
    void End() override;
    std::unique_ptr<mi::access::table::ITuple> Execute() override;
    ~SeqScan() override = default;
};
}; // namespace mi::executor::plan