#pragma once

#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include <optional>

namespace mi::executor {
class IExpressionNode {
  public:
    virtual std::optional<Datum> Exec(access::table::ITuple &tuple) = 0;
    virtual ~IExpressionNode() = default;

    bool ExecQual(access::table::ITuple &tuple) {
      auto value = this->Exec(tuple);
      return value.has_value() && value->getScalar<bool>();
    }
};
} // namespace mi::executor