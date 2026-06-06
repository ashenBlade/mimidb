#pragma once

#include "logger/ILogFormatter.hpp"

namespace mi::logger {
class DefaultLogFormatter : public ILogFormatter {
  public:
    std::string FormatLogMessage(LogLevel level, const char *format, std::va_list args) override;
    ~DefaultLogFormatter() override = default;
};
} // namespace mi::logger
