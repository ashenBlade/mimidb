#pragma once

#include "access/table/ITable.hpp"
#include "access/table/ITuple.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>
namespace mi::executor::plan {

class InsertNode : public IPlanNode {
  private:
    // Table where insert tuple
    access::table::ITable *_table;
    // Tuple to insert
    std::vector<std::unique_ptr<access::table::ITuple>> _tuples;

  public:
    InsertNode(access::table::ITable *table, std::vector<std::unique_ptr<access::table::ITuple>> tuple);
    void Start() override;
    void End() override;
    std::unique_ptr<access::table::ITuple> Execute() override;
};

} // namespace mi::executor::plan
