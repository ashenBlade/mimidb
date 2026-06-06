#pragma once

#include <optional>
#include <string>

namespace mi::client {

// Interface representing some source of commands to be executed by client
class ICommandSource {
  public:
    virtual std::optional<std::string> GetNextLine() = 0;
    virtual bool IsInteractive() = 0;

    virtual ~ICommandSource() = default;
};

//
class FDCommandSource : public ICommandSource {
  private:
    // File descriptor of file to read
    int _file;
    // Whether this file descriptor is for interactive stdin session
    bool _interactive;

  public:
    FDCommandSource(int file, bool interactive);

    std::optional<std::string> GetNextLine() override;
    bool IsInteractive() override;
};

class SingleCommandCommandSource : public ICommandSource {
  private:
    // Command to return
    std::string _command;
    // We have already executed
    bool _executed;

  public:
    SingleCommandCommandSource(std::string command);

    std::optional<std::string> GetNextLine() override;
    bool IsInteractive() override;
};

} // namespace mi::client