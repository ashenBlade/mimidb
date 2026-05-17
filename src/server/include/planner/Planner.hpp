#pragma once

#include "executor/plan/IPlanNode.hpp"
#include "sql/SQLStatement.h"
#include <memory>
namespace mi::planner {

class Planner {
  public:
    // Plan single statement
    static std::unique_ptr<executor::plan::IPlanNode> Plan(hsql::SQLStatement &statement);
    // This node can be planned using 'Plan'
    static bool IsPlannableStatement(hsql::SQLStatement &statement);
};

} // namespace mi::planner
