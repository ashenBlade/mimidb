#include "FileLogHandler.hpp"
#include "storage/io/File.hpp"

using namespace mi::logger;

FileLogHandler::FileLogHandler(mi::storage::io::File file): _file(std::move(file)) {};

void FileLogHandler::Write(const std::string &message) {
    this->_file.Append(reinterpret_cast<const std::byte *>(message.c_str()), message.size());
}

FileLogHandler::~FileLogHandler() {
    // Before closing file flush all pending log messages
    this->_file.Fsync();
    this->_file.Close();
}
