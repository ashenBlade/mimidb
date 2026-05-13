#pragma once

#include "access/table/AttrNumber.hpp"
#include "access/table/ITable.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include "trans/Snapshot.hpp"
#include <memory>
namespace mi::executor::plan {

class UpdateNode : public IPlanNode {
  private:
    // Table where insert tuple
    access::table::ITable *_table;
    // Predicate for tuples
    std::unique_ptr<IExpressionNode> _qual;
    // Attributes and their updates to perform
    std::vector<std::pair<access::table::AttrNumber, std::unique_ptr<IExpressionNode>>> _updates;
    // Snapshot for scanning table
    storage::trans::Snapshot *_snapshot;
    // Scan for current table
    std::unique_ptr<access::table::ITableScan> _scan;

  public:
    UpdateNode(access::table::ITable *table, std::unique_ptr<IExpressionNode> qual,
               storage::trans::Snapshot *snapshot,
               std::vector<std::pair<access::table::AttrNumber, std::unique_ptr<IExpressionNode>>>
                   updates);
    void Start() override;
    void End() override;
    std::unique_ptr<access::table::ITuple> Execute() override;
};

} // namespace mi::executor::plan
