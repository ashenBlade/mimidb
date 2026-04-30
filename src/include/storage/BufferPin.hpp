#pragma once

#include <memory>

#include "storage/Buffer.hpp"
#include "storage/PageNumber.hpp"
#include "storage/PageTag.hpp"

namespace mi::storage {

/// @brief RAII wrapper above Buffer for pin/unpin logic (for locking see Buffer[Shared]Lock)
class BufferPin {
  private:
    /// @brief 
    PageTag _tag;

    /// @brief Pinned buffer
    std::shared_ptr<Buffer> _buffer;

    public:
    // Create already initialized buffer
    BufferPin(PageTag tag, std::shared_ptr<Buffer> buffer);

    // Invalid buffer
    BufferPin();

    PageTag GetPageTag() const;
    std::shared_ptr<Buffer> GetBuffer();
    std::shared_ptr<Buffer> GetBuffer() const;

    std::byte *GetContents();
    const std::byte *GetContents() const;

    BufferPin(BufferPin &&other);
    BufferPin &operator=(BufferPin &&other);

    // We can not (and should not) copy buffer pins, max is to move
    BufferPin(const BufferPin &other) = delete;
    BufferPin &operator=(const BufferPin &other) = delete;

    /// @brief At the end return this to buffer pool and unpin
    ~BufferPin();
};
}; // namespace mi::storage