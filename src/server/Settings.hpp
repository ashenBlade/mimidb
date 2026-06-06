#pragma once

#include <string>
namespace mi {

struct Settings {
    // Filename for log file
    std::string LogFile;

    // Path to database data directory
    std::string DataDirectory;
};

Settings parseCommandArgs(int argc, const char **argv);

} // namespace mi
