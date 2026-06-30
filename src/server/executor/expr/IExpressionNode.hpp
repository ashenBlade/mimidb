#pragma once

#include "access/ITuple.hpp"
#include "executor/Datum.hpp"
#include <optional>

namespace mi::executor {
class IExpressionNode {
  public:
    virtual std::optional<Datum> Exec(access::ITuple &tuple) = 0;
    virtual ~IExpressionNode() = default;

    bool ExecQual(access::ITuple &tuple) {
      auto value = this->Exec(tuple);
      return value.has_value() && value->getScalar<bool>();
    }
};
} // namespace mi::executor