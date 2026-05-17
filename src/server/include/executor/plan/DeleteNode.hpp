#pragma once

#include "access/table/ITable.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include "trans/Snapshot.hpp"
#include <memory>
namespace mi::executor::plan {

class DeleteNode : public IPlanNode {
  private:
    // Table where to delete tuples
    access::table::ITable *_table;
    // Tuple predicate
    std::unique_ptr<IExpressionNode> _qual;
    // Snapshot for scan
    storage::trans::Snapshot *_snapshot;
    // Scan state
    std::unique_ptr<access::table::ITableScan> _scan;

  public:
    DeleteNode(access::table::ITable *table, std::unique_ptr<IExpressionNode> qual);
    void Start(storage::trans::Snapshot *snapshot) override;
    void End() override;
    std::unique_ptr<access::table::ITuple> Execute() override;
};

} // namespace mi::executor::plan
