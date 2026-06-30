#pragma once

#include "access/ITable.hpp"
#include "access/ITuple.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>
namespace mi::executor::plan {

class InsertNode : public PlanNode {
  private:
    // Table where insert tuple
    access::ITable *_table;
    // Tuple to insert
    std::vector<std::unique_ptr<access::ITuple>> _tuples;

  public:
    InsertNode(access::ITable *table, std::vector<std::unique_ptr<access::ITuple>> tuple);
    void Start(storage::trans::Snapshot *snapshot) override;
    void End() override;
    std::unique_ptr<access::ITuple> Execute() override;
};

} // namespace mi::executor::plan
