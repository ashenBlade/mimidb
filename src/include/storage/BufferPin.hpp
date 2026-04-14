#pragma once

#include <memory>

#include "storage/Buffer.hpp"
#include "storage/PageNumber.hpp"

namespace mi::storage {

/// @brief RAII wrapper above Buffer for pin/unpin logic (for locking see Buffer[Shared]Lock)
class BufferPin {
  private:
    /// @brief Number of
    PageNumber _pageno;
    std::shared_ptr<Buffer> _buffer;

    // Create already initialized buffer
    BufferPin(PageNumber pageno, std::shared_ptr<Buffer> buffer);

  public:
    // Invalid buffer
    BufferPin();
    
    PageNumber GetPageNumber() const;

    std::byte *GetContents();
    const std::byte *GetContents() const;

    BufferPin(BufferPin &&other);
    BufferPin &operator=(const BufferPin &&other);

    // We can not (and should not) copy buffer pins, max is to move
    BufferPin(const BufferPin &other) = delete;
    BufferPin &operator=(const BufferPin &other) = delete;

    /// @brief At the end return this to buffer pool and unpin
    ~BufferPin();

    static BufferPin GetBuffer(PageNumber pageno);
};
}; // namespace mi::storage