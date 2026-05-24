#pragma once

#include "sql/SQLStatement.h"

namespace mi::parser {

class SQLParser {
  public:
    static std::unique_ptr<hsql::SQLStatement> ParseStatement(const std::string &query);
};

} // namespace mi::parser