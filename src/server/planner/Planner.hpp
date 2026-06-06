#pragma once

#include "planner/PlannedStmt.hpp"
#include "sql/SQLStatement.h"
namespace mi::planner {

class Planner {
  public:
    // Plan single statement
    static PlannedStmt Plan(hsql::SQLStatement &statement);
    // This node can be planned using 'Plan'
    static bool IsPlannableStatement(hsql::SQLStatement &statement);
};

} // namespace mi::planner
