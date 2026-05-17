#pragma once

#include "access/table/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/func/FunctionContext.hpp"
#include <memory>
#include <vector>

namespace mi::executor {
class FunctionExpressionNode : public IExpressionNode {
  private:
    // Function to invoke
    FunctionContext _funcCtx;
    // For each attribute represents arguments to be evaluated to get function argument
    std::vector<std::unique_ptr<IExpressionNode>> _arguments;

  public:
    FunctionExpressionNode(
      FunctionContext context, std::vector<std::unique_ptr<IExpressionNode>> arguments);
    std::optional<Datum> Exec(access::table::ITuple &tuple) override;
    ~FunctionExpressionNode() override = default;
};
} // namespace mi::executor
