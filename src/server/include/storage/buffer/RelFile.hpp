#pragma once

#include "access/table/Oid.hpp"
#include "storage/buffer/PageNumber.hpp"
#include "storage/io/File.hpp"

namespace mi::storage {
// Wrapper object for work with page-based files (relations)
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
    
    // Extend relation file up to specified page (including) with zeroes
    void Extend(PageNumber pageno);

    // Read single page
    void Read(std::byte *buffer, PageNumber pageno);
    
    // Perform fsync
    void Flush();
    
    // Get number of pages in given relation
    PageNumber GetPagesCount();

    // Close this RelFile releasing all resources
    void Close();
    
    /// Open new relation file in specified mode
    /// @param mode File mode to open file with
    static RelFile Open(Oid relid, int mode);
};
} // namespace mi::storage