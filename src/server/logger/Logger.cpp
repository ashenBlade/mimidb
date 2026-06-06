#include "logger/Logger.hpp"

#include <cstdio>
#include <cstring>
#include <mutex>

using namespace mi::logger;

Logger::Logger(std::unique_ptr<ILogHandler> handler, std::unique_ptr<ILogFormatter> formatter)
    : _latch(), _handler(std::move(handler)), _formatter(std::move(formatter)) {};

void Logger::log(LogLevel level, const char *fmt, va_list args) {
    auto msg = this->_formatter->FormatLogMessage(level, fmt, args);

    auto g = std::unique_lock{this->_latch};
    this->_handler->Write(msg);
}

void Logger::Debug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    this->log(LogLevel::DEBUG, msg, args);
    va_end(args);
}

void Logger::Info(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    this->log(LogLevel::INFO, msg, args);
    va_end(args);
}

void Logger::Warning(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    this->log(LogLevel::WARNING, msg, args);
    va_end(args);
}

void Logger::Error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    this->log(LogLevel::ERROR, msg, args);
    va_end(args);
}
