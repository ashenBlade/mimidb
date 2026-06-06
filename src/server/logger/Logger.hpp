#pragma once

#include "lock/LWLatch.hpp"
#include "logger/ILogFormatter.hpp"
#include "logger/ILogHandler.hpp"
#include "logger/LogLevel.hpp"
#include <cstdarg>
#include <memory>
namespace mi::logger {
class Logger {
  private:
    // Mutex to synchronize access
    lock::LWLatch _latch;
    // Actual handler that performs logging
    std::unique_ptr<ILogHandler> _handler;
    // Formatter to get log message
    std::unique_ptr<ILogFormatter> _formatter;

    void log(LogLevel level, const char *msg, va_list args);

  public:
    Logger(std::unique_ptr<ILogHandler> handler, std::unique_ptr<ILogFormatter> formatter);

    void Debug(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Info(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Warning(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Error(const char *msg, ...) __attribute__((format(printf,2,3)));
};
} // namespace mi::logger
