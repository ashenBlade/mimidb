#pragma once

#include "storage/buffer/Buffer.hpp"
#include "storage/buffer/PageTag.hpp"
#include <memory>

namespace mi::storage::buffer {

/// @brief RAII wrapper above Buffer for pin/unpin logic (for locking see Buffer[Shared]Lock)
class BufferPin {
  private:
    /// @brief
    PageTag _tag;

    /// @brief Pinned buffer
    Buffer *_buffer;

  public:
    // Create already initialized buffer
    BufferPin(PageTag tag, Buffer *buffer);

    // Invalid buffer
    BufferPin();

    PageTag GetPageTag() const { return this->_tag; }

    Buffer *GetBuffer() { return this->_buffer; }
    Buffer *GetBuffer() const { return this->_buffer; }

    bool IsValid() const { return this->_buffer != nullptr; }

    std::byte *GetContents() { return this->_buffer->GetContents(); }
    const std::byte *GetContents() const { return this->_buffer->GetContents(); }

    BufferPin(BufferPin &&other);
    BufferPin &operator=(BufferPin &&other);

    // We can not (and should not) copy buffer pins, max is to move
    BufferPin(const BufferPin &other) = delete;
    BufferPin &operator=(const BufferPin &other) = delete;

    /// @brief At the end return this to buffer pool and unpin
    ~BufferPin();
};
}; // namespace mi::storage::buffer