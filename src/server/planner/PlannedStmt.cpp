#include "PlannedStmt.hpp"
#include "executor/plan/CommandTag.hpp"
#include "executor/plan/IPlanNode.hpp"
#include <memory>

using namespace mi::planner;
using namespace mi::executor::plan;

PlannedStmt::PlannedStmt(std::unique_ptr<PlanNode> plan, CommandTag tag)
    : _plan(std::move(plan)), _tag(tag) {};

PlanNode *PlannedStmt::GetNode() { return this->_plan.get(); }

CommandTag PlannedStmt::GetCmdTag() { return this->_tag; }
