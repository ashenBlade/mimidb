#pragma once

#include <cstddef>
#include <unistd.h>

namespace mi::storage {

// Wrapper above file descriptor 
class File {
  private:
    /// @brief File descriptor
    int _fd;

  public:
    File();
    File(int fd);

    // Copying is not supported
    File(const File &other) = delete;
    File &operator=(File &other) = delete;

    File(File &&other) noexcept;
    File &operator=(File &&other) noexcept;

    /// @brief Write given buffer to file
    /// @param buffer Buffer with data
    /// @param size Size of buffer
    /// @param offset Offset in file at which to start writing
    void Write(const std::byte *buffer, size_t size, off64_t offset);

    /// @brief Read from file into given buffer
    /// @param buffer Buffer to which write data
    /// @param size Size of data to read
    /// @param offset Offset at which to start reading
    /// @return Bytes actually read if torn page encountered. If error exception is thrown. If EOF - 0 is returned.
    size_t Read(std::byte *buffer, size_t size, off64_t offset);

    /// @brief Flush data to disk, same as fsync
    void Fsync();

    /// @brief Get size of file
    /// @return Size of file
    off64_t Size();

    /// @brief Close file
    void Close();

    ~File();
};

}; // namespace mi::storage
