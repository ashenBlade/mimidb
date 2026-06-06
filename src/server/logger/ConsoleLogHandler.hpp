#pragma once

#include "logger/ILogHandler.hpp"

namespace mi::logger {

class ConsoleLogHandler : public ILogHandler {
  public:
    void Write(const std::string &message) override;

    ~ConsoleLogHandler() override;
};

} // namespace mi::logger
