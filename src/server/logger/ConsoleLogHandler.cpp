#include "ConsoleLogHandler.hpp"
#include <iostream>

using namespace mi::logger;

void ConsoleLogHandler::Write(const std::string &message) {
    std::cout << message << std::endl;
}

ConsoleLogHandler::~ConsoleLogHandler() {
    // Flush pending data
    std::cout << std::flush;
}

