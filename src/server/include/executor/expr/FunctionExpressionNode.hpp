#pragma once

#include "access/table/AttrNumber.hpp"
#include "access/table/ITuple.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/func/FunctionContext.hpp"
#include "utils/DatumArray.hpp"
#include <utility>
#include <vector>

namespace mi::executor {
class FunctionExpressionNode : public IExpressionNode {
  private:
    // Function to invoke
    FunctionContext _funcCtx;
    // Array of constant values to place
    DatumArray _constants;
    // Indexes of arguments for constant values
    std::vector<size_t> _constantMapping;
    // Mapping between attribute number in input tuple to function argument number
    std::vector<std::pair<access::table::AttrNumber, size_t>> _attributeMapping;
    DatumArray createFunctionArgs(access::table::ITuple &tuple);

  public:
    FunctionExpressionNode(
        FunctionContext context, DatumArray constants, std::vector<size_t> constantMapping,
        std::vector<std::pair<access::table::AttrNumber, size_t>> attributeMapping);
    std::optional<Datum> Exec(access::table::ITuple &tuple) override;
    ~FunctionExpressionNode() override = default;
};
} // namespace mi::executor
