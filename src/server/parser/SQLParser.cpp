#include "parser/SQLParser.hpp"

#include "SQLParser.h"
#include "SQLParserResult.h"
#include "sql/SQLStatement.h"
#include <stdexcept>
#include <string>

using namespace mi::parser;

std::unique_ptr<hsql::SQLStatement> SQLParser::ParseStatement(const std::string &query) {
    hsql::SQLParserResult result{};
    if (!hsql::SQLParser::parse(query, &result)) {
        throw std::runtime_error("could not parse statement");
    }

    if (!result.isValid()) {
        throw std::runtime_error("could not parse statement: " + std::string{result.errorMsg()});
    }

    if (result.size() != 1) {
        throw std::runtime_error("could not parse statement: only 1 statement at a time supported");
    }

    return std::unique_ptr<hsql::SQLStatement>{result.extractStatement(0)};
}
