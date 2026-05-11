#include "mimidb.hpp"

#include "storage/io/File.hpp"

#include <cstdio>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <format>

using namespace mi::storage;

constexpr const int InvalidFd = -1;

File::File(): _fd(InvalidFd) {};

File::File(int fd): _fd(fd) {};

File::File(File &&other) noexcept {
    assert(this != &other);
    this->_fd = other._fd;
    other._fd = InvalidFd;
}

File &File::operator=(File &&other) noexcept {
    assert(this != &other);
    this->_fd = other._fd;
    other._fd = InvalidFd;
    return *this;
}

static inline void throw_error(const char *op) {
    auto what = std::format("could not {} file", op);
    throw std::system_error(std::error_code{errno, std::system_category()}, std::move(what));
}

void File::Write(const std::byte *buffer, size_t size, off64_t offset) {
    auto ret = pwrite64(this->_fd, static_cast<const void *>(buffer), size, offset);
    if (ret < 0) {
        throw_error("write");
    }
}

size_t File::Read(std::byte *buffer, size_t size, off64_t offset) {
    auto ret = pread64(this->_fd, static_cast<void *>(buffer), size, offset);
    if (ret < 0) {
        throw_error("read");
    }
    
    return static_cast<size_t>(ret);
}

void File::Fsync() {
    auto ret = fdatasync(this->_fd);
    if (ret < 0) {
        throw_error("fsync");
    }
}

off64_t File::Size() {
    auto ret = lseek64(this->_fd, 0, SEEK_END);
    if (ret < 0) {
        throw_error("lseek");
    }

    return ret;
}

void File::Close() {
    if (this->_fd != InvalidFd) {
        close(this->_fd);
        this->_fd = InvalidFd;
    }
}

File::~File() {
    this->Close();
}

File File::Open(const std::string &path, int mode) {
    auto fd = open(path.c_str(), mode, 0666);
    if (fd < 0) {
        throw std::runtime_error("could not open file");
    }

    return File{fd};
}
