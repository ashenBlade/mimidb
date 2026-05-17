#pragma once

#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include <optional>
namespace mi::executor::expr {

class ConstantExpressionNode : public IExpressionNode {
  private:
    std::optional<Datum> _value;

  public:
    ConstantExpressionNode(std::optional<Datum> value) : _value(value) {}
    std::optional<Datum> Exec([[maybe_unused]] access::table::ITuple &tuple) override {
        return this->_value;
    }
    ~ConstantExpressionNode() override = default;
};

} // namespace mi::executor::expr
