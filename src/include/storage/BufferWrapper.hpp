#pragma once

#include <memory>

#include "storage/Buffer.hpp"
#include "storage/BufferManager.hpp"
#include "storage/PageNumber.hpp"

namespace mi::storage {

/// @brief RAII wrapper above Buffer
class BufferWrapper {
  private:
    /// @brief Number of
    PageNumber _pageno;
    std::shared_ptr<Buffer> _buffer;
    BufferManager &_manager;
    bool _isLocked;
    bool _lockIsShared;

    BufferWrapper(PageNumber pageno, std::shared_ptr<Buffer> buffer, BufferManager &manager);

  public:

    std::byte *GetContents();
    const std::byte *GetContents() const;
    void Lock(bool shared);
    void Unlock();

    /// @brief At the end return this to buffer pool and unpin
    ~BufferWrapper();
};
}; // namespace mi::storage