#include "executor/expr/BoolExpressionNode.hpp"
#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include <stdexcept>

using namespace mi::executor::expr;

std::optional<mi::Datum> BoolExpressionNode::Exec(mi::access::table::ITuple &tuple) {
    assert(this->_expressions.size() > 0);

    // Choose initial value
    bool defaultValue;
    switch (this->_op) {
        case mi::executor::expr::BoolOp::And:
            defaultValue = true;
            break;
        case mi::executor::expr::BoolOp::Or:
            defaultValue = false;
            break;
        case mi::executor::expr::BoolOp::Not:
            // for not we return first result as it appears, but
            // anyway init it to keep compiler quiet
            defaultValue = false;
            break;
    }

    for (auto &expr : this->_expressions) {
        auto result = expr->Exec(tuple);

        // Boolean operations are strict, so return NULL as we encounter one
        if (!result.has_value()) {
            return std::nullopt;
        }

        auto val = result.value().getScalar<bool>();
        switch (this->_op) {
        case mi::executor::expr::BoolOp::And:
            if (!val) {
                return Datum{false};
            }
            break;
        case mi::executor::expr::BoolOp::Or:
            if (val) {
                return Datum{true};
            }
            break;
        case mi::executor::expr::BoolOp::Not:
            // For NOT we must have only 1 expression
            return result.value();
        }
    }

    return Datum{defaultValue};
}
