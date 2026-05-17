#include "executor/expr/FunctionExpressionNode.hpp"
#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include "utils/DatumArray.hpp"
#include <optional>

using namespace mi::executor;

FunctionExpressionNode::FunctionExpressionNode(
    FunctionContext context, std::vector<std::unique_ptr<IExpressionNode>> arguments)
    : _funcCtx(std::move(context)), _arguments(std::move(arguments)) {};

std::optional<mi::Datum> FunctionExpressionNode::Exec(mi::access::table::ITuple &tuple) {
    // Build arguments array for function
    auto array = DatumArray{};
    for (const auto &expr : this->_arguments) {
        auto arg = expr->Exec(tuple);
        array.Add(arg);
    }

    if (this->_funcCtx.IsStrict()) {
        // If any of attributes are null then return NULL immediately
        // Tuple attribute values
        for (auto i = 0U; i < array.Size(); ++i) {
            if (!array.Get(i).has_value()) {
                return std::nullopt;
            }
        }
    }

    return this->_funcCtx.Invoke(FunctionArgs{std::move(array)});
}
