#include "parser/SQLParser.hpp"

#include "SQLParser.h"
#include "SQLParserResult.h"
#include "parser/ParserError.hpp"
#include "sql/SQLStatement.h"
#include <string>

using namespace mi::parser;

std::unique_ptr<hsql::SQLStatement> SQLParser::ParseStatement(const std::string &query) {
    hsql::SQLParserResult result{};
    if (!hsql::SQLParser::parse(query, &result)) {
        throw ParserError("failed to initialize lex");
    }

    if (!result.isValid()) {
        throw ParserError(result.errorMsg());
    }

    if (result.size() != 1) {
        throw ParserError("only 1 statement at a time supported");
    }

    return std::unique_ptr<hsql::SQLStatement>{result.extractStatement(0)};
}
