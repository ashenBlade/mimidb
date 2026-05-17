#pragma once

#include "executor/Oid.hpp"
#include "executor/func/MIFunction.hpp"
namespace mi::db::catalog {

// Strategy operator represents
enum OperatorStrategy {
    Equal = 0,
    NotEqual,
    Less,
    LessEq,
    Greater,
    GreaterEq,
};

class OperatorInfo {
  private:
    // Id of operator
    Oid _id;
    // Left argument type id
    Oid _leftType;
    // Right argument type id
    Oid _rightType;
    // Operator strategy
    OperatorStrategy _strategy;
    // Actual function
    executor::MIFunction _function;

  public:
    OperatorInfo(Oid opId, Oid leftType, Oid rightType, OperatorStrategy operatorStrategy,
                 executor::MIFunction function)
        : _id(opId), _leftType(leftType), _rightType(rightType), _strategy(operatorStrategy),
          _function(function) {};
    Oid Id() const { return this->_id; }
    Oid LeftType() const { return this->_leftType; }
    Oid RightType() const { return this->_rightType; }
    OperatorStrategy Strategy() const { return this->_strategy; }
    executor::MIFunction GetFunction() const { return this->_function; }
};

} // namespace mi::db::catalog
