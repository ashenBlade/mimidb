#include "CommandSource.hpp"
#include <unistd.h>

using namespace mi::client;

FDCommandSource::FDCommandSource(int file, bool interactive)
    : _file(file), _interactive(interactive) {};

std::optional<std::string> FDCommandSource::GetNextLine() {
    // Read byte by byte until we read EOF
    while (true) {
        std::string command{};

        char c;
        while (read(this->_file, &c, sizeof(char)) == 1) {
            if (c == '\n') {
                // Skip all leading empty lines
                if (command.size() == 0) {
                    continue;
                } else {
                    break;
                }
            }

            command.push_back(c);
        }

        if (command.size() == 0) {
            return std::nullopt;
        } else {
            return std::optional{std::move(command)};
        }
    }
}

bool FDCommandSource::IsInteractive() { return this->_interactive; }

SingleCommandCommandSource::SingleCommandCommandSource(std::string command)
    : _command(std::move(command)), _executed(false) {}

std::optional<std::string> SingleCommandCommandSource::GetNextLine() {
    if (!this->_executed) {
        this->_executed = true;
        return this->_command;
    } else {
        return std::nullopt;
    }
}

bool SingleCommandCommandSource::IsInteractive() { return false; }
