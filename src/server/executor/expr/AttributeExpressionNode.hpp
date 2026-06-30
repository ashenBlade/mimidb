#pragma once

#include "access/AttrNumber.hpp"
#include "access/ITuple.hpp"
#include "executor/Datum.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include <optional>
namespace mi::executor::expr {
class AttributeExpressionNode : public IExpressionNode {
  private:
    access::AttrNumber _attno;

  public:
    AttributeExpressionNode(access::AttrNumber attno) : _attno(attno) {}
    std::optional<Datum> Exec(access::ITuple &tuple) override {
        return tuple.GetAttribute(this->_attno);
    }
    ~AttributeExpressionNode() override = default;
};
} // namespace mi::executor::expr
