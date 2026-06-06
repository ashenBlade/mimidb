#pragma once

#include "logger/ILogHandler.hpp"
#include "storage/io/File.hpp"
namespace mi::logger {

class FileLogHandler : public ILogHandler {
  private:
    // File to actually write data
    storage::io::File _file;

  public:
    FileLogHandler(storage::io::File file);
    void Write(const std::string &message) override;

    ~FileLogHandler() override;
};

} // namespace mi::logger
