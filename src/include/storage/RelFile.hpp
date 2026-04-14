#pragma once

#include "storage/File.hpp"
#include "storage/PageNumber.hpp"

namespace mi::storage {
class RelFile {
  private:
    File _file;

  public:
    RelFile(File &&file);
    
    // Copying is not supported
    RelFile(const RelFile &other) = delete;
    RelFile &operator=(File &other) = delete;

    RelFile(RelFile &&other) noexcept;
    RelFile &operator=(RelFile &&other) noexcept;

    ~RelFile();

    // Write single page
    void Write(const std::byte *buffer, PageNumber pageno);
    
    // Read single page
    void Read(std::byte *buffer, PageNumber pageno);
    
    // Perform fsync
    void Flush();
    
    // Get number of pages in given relation
    PageNumber GetPagesCount();

    // Close this RelFile releasing all resources
    void Close();
};
} // namespace mi::storage