#pragma once

#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include <optional>

namespace mi::executor {
class IExpressionNode {
  public:
    virtual std::optional<Datum> Exec(access::table::ITuple &tuple) = 0;
    virtual ~IExpressionNode() = default;
};
} // namespace mi::executor