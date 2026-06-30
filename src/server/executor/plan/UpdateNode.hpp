#pragma once

#include "access/AttrNumber.hpp"
#include "access/ITable.hpp"
#include "access/ITableScan.hpp"
#include "access/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/plan/IPlanNode.hpp"
#include "trans/Snapshot.hpp"
#include <memory>
namespace mi::executor::plan {

class UpdateNode : public PlanNode {
  private:
    // Table where insert tuple
    access::ITable *_table;
    // Predicate for tuples
    std::unique_ptr<IExpressionNode> _qual;
    // Attributes and their updates to perform
    std::vector<std::pair<access::AttrNumber, std::unique_ptr<IExpressionNode>>> _updates;
    // Scan for current table
    std::unique_ptr<access::ITableScan> _scan;

  public:
    UpdateNode(access::ITable *table, std::unique_ptr<IExpressionNode> qual,
               std::vector<std::pair<access::AttrNumber, std::unique_ptr<IExpressionNode>>>
                   updates);
    void Start(storage::trans::Snapshot *snapshot) override;
    void End() override;
    std::unique_ptr<access::ITuple> Execute() override;
};

} // namespace mi::executor::plan
