#include "Settings.hpp"
#include <boost/program_options.hpp>
#include <string>

using namespace mi;
using namespace boost::program_options;

Settings mi::parseCommandArgs(int argc, const char **argv) {
    Settings settings{};

    boost::program_options::options_description desc{"mimidb"};

    // clang-format off
    desc.add_options()
        ("logfile,f", value<std::string>(), "Log file name")
        ("datadir,D", value<std::string>()->required(), "Path to data directory")
    ;
    // clang-format on

    variables_map vm{};
    store(parse_command_line(argc, argv, desc), vm);

    auto it = vm.find("logfile");
    if (it != vm.end()) {
        settings.LogFile = it->second.as<std::string>();
    }

    it = vm.find("datadir");
    if (it != vm.end()) {
        settings.DataDirectory = it->second.as<std::string>();
    }

    return settings;
}