#pragma once

#include "access/table/AttrNumber.hpp"
#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include <optional>
namespace mi::executor::expr {
class AttributeExpressionNode : public IExpressionNode {
  private:
    access::table::AttrNumber _attno;

  public:
    AttributeExpressionNode(access::table::AttrNumber attno) : _attno(attno) {}
    std::optional<Datum> Exec(access::table::ITuple &tuple) override {
        return tuple.GetAttribute(this->_attno);
    }
    ~AttributeExpressionNode() override = default;
};
} // namespace mi::executor::expr
