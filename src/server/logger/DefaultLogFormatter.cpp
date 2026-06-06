#include "DefaultLogFormatter.hpp"
#include <array>
#include <cstring>

using namespace mi::logger;

static size_t format_msg(LogLevel level, const char *msg, va_list args, char *output, size_t bufferSize) {
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

    // Copy log level
    std::memcpy(output, levelMsg, levelMsgLen);
    bufferSize -= levelMsgLen;
    output += levelMsgLen;

    // Message itself
    auto count = std::vsnprintf(output, bufferSize, msg, args);
    return levelMsgLen + static_cast<size_t>(count);
}

constexpr inline auto MaxLogMessageSize = 1024;

std::string DefaultLogFormatter::FormatLogMessage(LogLevel level, const char *format, std::va_list args) {
    auto buffer = std::array<char, MaxLogMessageSize>{};
    auto size = format_msg(level, format, args, buffer.data(), buffer.size());
    return std::string{buffer.data(), size};
}