#pragma once

#include <ctime>
#include <string>

namespace mi::logger {
class ILogHandler {
  public:
    virtual void Write(const std::string &message) = 0;

    virtual ~ILogHandler() = default;
};
} // namespace mi::logger
