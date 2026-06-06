#pragma once

#include "logger/LogLevel.hpp"
#include <cstdarg>
#include <string>

namespace mi::logger {

class ILogFormatter {
  public:
    virtual std::string FormatLogMessage(LogLevel level, const char *format, std::va_list args) = 0;
    virtual ~ILogFormatter() = default;
};

} // namespace mi::logger
