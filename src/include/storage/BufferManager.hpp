#pragma once

#include <memory>
#include <vector>

#include "storage/Buffer.hpp"
#include "storage/PageNumber.hpp"

namespace mi::storage {
class BufferManager {
  private:
    /// @brief All fetched pages indexed by page number
    std::vector<std::shared_ptr<Buffer>> _pages;
    /// @brief For for updating pages array
    std::shared_mutex _mutex;

  public:
    BufferManager();

    /// @brief Get buffer for given page number
    /// @param pageno Number of page to get
    /// @return Buffer entry to get access to buffer and it's contents
    std::shared_ptr<Buffer> GetBuffer(PageNumber pageno);

    /// @brief Return buffer to pool
    /// @param buffer
    void ReturnBuffer(std::shared_ptr<Buffer> buffer, PageNumber pageno);

    ~BufferManager();
};
}; // namespace mi::storage
