#pragma once

#include "access/ITable.hpp"
#include "access/ITableScan.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include "trans/Snapshot.hpp"
#include <memory>

namespace mi::executor::plan {
class SeqScan : public PlanNode {
  private:
    /// @brief State for scanning node
    std::unique_ptr<mi::access::ITableScan> _scan;
    /// @brief Table we are going to scan
    mi::access::ITable *_table;
    /// @brief Snapshot to use during scan
    mi::storage::trans::Snapshot *_snapshot;
    /// @brief Predicate for SELECT
    std::unique_ptr<IExpressionNode> _qual;

  public:
    SeqScan(mi::access::ITable *table, std::unique_ptr<IExpressionNode> qual);

    void Start(storage::trans::Snapshot *snapshot) override;
    void End() override;
    std::unique_ptr<mi::access::ITuple> Execute() override;
    ~SeqScan() override = default;
};
}; // namespace mi::executor::plan