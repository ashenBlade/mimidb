#pragma once

#include "lock/LWLatch.hpp"
#include <cstdarg>
namespace mi::logger {
enum LogLevel {
    OFF = 0,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

class Logger {
  private:
    // Mutex to synchronize access
    lock::LWLatch _latch;

    void log(LogLevel level, const char *msg, va_list args);
  public:
    Logger();
    void Debug(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Info(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Warning(const char *msg, ...) __attribute__((format(printf,2,3)));
    void Error(const char *msg, ...) __attribute__((format(printf,2,3)));
};
} // namespace mi::logger
