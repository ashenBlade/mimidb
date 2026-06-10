#include "CommandDest.hpp"

#include <cstring>
#include <stdexcept>
#include <unistd.h>

using namespace mi::client;

FileCommandDest::FileCommandDest(int fd): _fd(fd) {};

void FileCommandDest::Write(const std::string &value) {
    auto cursor = value.data();
    auto left = value.size();
    while (left > 0) {
        auto ret = write(this->_fd, cursor, left);
        if (ret < 0) {
            std::string errmsg = "could not write to file: ";
            errmsg += strerror(errno);
            throw std::runtime_error(errmsg);
        }

        cursor += static_cast<size_t>(ret);
        left -= static_cast<size_t>(ret);
    }
}
