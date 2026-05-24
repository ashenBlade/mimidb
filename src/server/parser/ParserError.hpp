#pragma once

#include <stdexcept>
#include <string>
namespace mi::parser {
class ParserError : public std::runtime_error {
    public:
        ParserError(const std::string &msg): std::runtime_error(msg) {};
};
} // namespace mi::parser
