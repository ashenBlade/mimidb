#pragma once

#include <optional>
#include <string>

namespace mi::client {
struct ClientOptions {
    // SQL filename to execute commands from
    std::optional<std::string> ScriptFile;

    // Single SQL statement to execute
    std::optional<std::string> SingleCommand;

    // File to write output instead of stdout
    std::optional<std::string> OutputFile;

    // Port to connect
    std::optional<int> Port;

    // Host to connect
    std::optional<std::string> Host;

    // Show version requested
    bool Version;

    static ClientOptions ParseOptions(int argc, const char **argv);
};
} // namespace mi::client
