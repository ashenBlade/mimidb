#pragma once

#include "executor/plan/CommandTag.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>
namespace mi::planner {
class PlannedStmt {
  private:
    // Actual plan node
    std::unique_ptr<executor::plan::PlanNode> _plan;
    executor::plan::CommandTag _tag;

  public:
    PlannedStmt(std::unique_ptr<executor::plan::PlanNode> plan, executor::plan::CommandTag tag);

    executor::plan::PlanNode *GetNode();
    executor::plan::CommandTag GetCmdTag();
};
} // namespace mi::executor::plan
