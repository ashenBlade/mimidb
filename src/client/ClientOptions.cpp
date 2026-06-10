#include "ClientOptions.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <stdexcept>

using namespace mi::client;
using namespace boost::program_options;

ClientOptions ClientOptions::ParseOptions(int argc, const char **argv) {
    boost::program_options::options_description desc{"Options"};

    // clang-format off
    desc.add_options()
        ("version,v", "Get version")
        ("command,c", boost::program_options::value<std::string>(), "Command to execute")
        ("file,f", boost::program_options::value<std::string>(), "SQL file to execute commands from")
        ("port,p", value<int>(), "Database port to connect")
        ("host,h", value<std::string>(), "Database host to connect")
        ("output,o", value<std::string>(), "File to write output")
    ;
    // clang-format on

    boost::program_options::variables_map vm{};
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

    ClientOptions options{};

    auto it = vm.find("version");
    if (it != vm.end()) {
        // Shortcut when only version is requested
        options.Version = true;
        return options;
    }

    it = vm.find("command");
    if (it != vm.end()) {
        options.SingleCommand = it->second.as<std::string>();
    }

    it = vm.find("file");
    if (it != vm.end()) {
        options.ScriptFile = it->second.as<std::string>();
    }

    it = vm.find("port");
    if (it != vm.end()) {
        options.Port = it->second.as<int>();
    }

    it = vm.find("host");
    if (it != vm.end()) {
        options.Host = it->second.as<std::string>();
    }

    it = vm.find("output");
    if (it != vm.end()) {
        options.OutputFile = it->second.as<std::string>();
    }

    // Parameter validation
    if (options.ScriptFile.has_value() && options.SingleCommand.has_value()) {
        throw std::runtime_error("can not specify both file and command");
    }

    return options;
}