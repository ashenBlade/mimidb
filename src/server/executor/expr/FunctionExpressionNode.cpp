#include "executor/expr/FunctionExpressionNode.hpp"
#include "access/table/ITuple.hpp"
#include "executor/Datum.hpp"
#include "utils/DatumArray.hpp"
#include <optional>

using namespace mi::executor;

FunctionExpressionNode::FunctionExpressionNode(
    FunctionContext context, DatumArray constants, std::vector<size_t> constantMapping,
    std::vector<std::pair<access::table::AttrNumber, size_t>> attributeMapping)
    : _funcCtx(std::move(context)), _constants(std::move(constants)),
      _constantMapping(std::move(constantMapping)),
      _attributeMapping(std::move(attributeMapping)) {};

mi::DatumArray FunctionExpressionNode::createFunctionArgs(mi::access::table::ITuple &tuple) {
    auto args = DatumArray{this->_funcCtx.GetNArgs()};

    // Set constant values
    auto nargs = this->_constantMapping.size();
    for (auto i = 0U; i < nargs; i++) {
        args.Set(this->_constantMapping[i], this->_constants.Get(i));
    }

    // Tuple attribute values
    for (const auto [attno, argno] : this->_attributeMapping) {
        args.Set(argno, tuple.GetAttribute(attno));
    }

    return args;
}

std::optional<mi::Datum> FunctionExpressionNode::Exec(mi::access::table::ITuple &tuple) {
    if (this->_funcCtx.IsStrict()) {
        // If any of attributes are null then return NULL immediately
        // Tuple attribute values
        for (const auto [attno, argno] : this->_attributeMapping) {
            auto value = tuple.GetAttribute(attno);
            if (!value.has_value()) {
                return std::nullopt;
            }
        }
    }

    return this->_funcCtx.Invoke(FunctionArgs{this->createFunctionArgs(tuple)});
}
