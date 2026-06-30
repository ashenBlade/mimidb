#pragma once

#include "access/ITable.hpp"
#include "access/ITableScan.hpp"
#include "access/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include "trans/Snapshot.hpp"
#include <memory>
namespace mi::executor::plan {

class DeleteNode : public PlanNode {
  private:
    // Table where to delete tuples
    access::ITable *_table;
    // Tuple predicate
    std::unique_ptr<IExpressionNode> _qual;
    // Scan state
    std::unique_ptr<access::ITableScan> _scan;

  public:
    DeleteNode(access::ITable *table, std::unique_ptr<IExpressionNode> qual);
    void Start(storage::trans::Snapshot *snapshot) override;
    void End() override;
    std::unique_ptr<access::ITuple> Execute() override;
};

} // namespace mi::executor::plan
