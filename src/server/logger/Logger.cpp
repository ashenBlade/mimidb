#include "logger/Logger.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>

using namespace mi::logger;

Logger::Logger() : _mutex() {};

static void format_msg(LogLevel level, const char *msg, va_list args, char *output, size_t length) {
    // Log Level
    const char *levelMsg;
    size_t levelMsgLen;
    switch (level) {
    case LogLevel::DEBUG:
        levelMsg = "[DEBUG] ";
        levelMsgLen = 8;
        break;
    case LogLevel::INFO:
        levelMsg = "[INFO] ";
        levelMsgLen = 7;
        break;
    case LogLevel::WARNING:
        levelMsg = "[WARN] ";
        levelMsgLen = 7;
        break;
    case LogLevel::ERROR:
        levelMsg = "[ERROR] ";
        levelMsgLen = 8;
        break;
    default:
        levelMsg = "[???] ";
        levelMsgLen = 6;
        break;
    }
    std::memcpy(output, levelMsg, levelMsgLen);
    length -= levelMsgLen;
    output += levelMsgLen;

    // Message itself
    auto count = std::vsnprintf(output, length, msg, args);
    output[count] = '\0';
}

void Logger::log(LogLevel level, const char *msg, va_list args) {
    auto message = std::array<char, 1024>{};
    format_msg(level, msg, args, message.data(), message.size());

    auto g = std::lock_guard{this->_mutex};
    // Write only under the lock
    std::cout << message.data() << std::endl;
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
