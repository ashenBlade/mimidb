#pragma once

#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
namespace mi::executor::expr {
// Different boolean operators
enum BoolOp {
    And,
    Or,
    Not,
};

// Executes different
class BoolExpressionNode : public IExpressionNode {
  private:
    // Expressions to evaluate
    std::vector<std::unique_ptr<IExpressionNode>> _expressions;
    // Boolean operation
    BoolOp _op;

  public:
    BoolExpressionNode(std::vector<std::unique_ptr<IExpressionNode>> expressions, BoolOp op)
        : _expressions(std::move(expressions)), _op(op) {
        if (this->_op == BoolOp::Not && this->_expressions.size() != 1) {
            throw std::runtime_error("For NOT only 1 expression is allowed");
        }
        if (this->_expressions.size() == 0) {
            throw std::runtime_error("Expression array does not contain any element");
        }
    };
    std::optional<Datum> Exec(access::table::ITuple &tuple) override;
    ~BoolExpressionNode() override = default;
};
} // namespace mi::executor::expr
