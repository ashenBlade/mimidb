#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace mi::storage::buffer {

// Forward declaration
class BufferManager;
class CacheEntry;

/// @brief Represents page in buffer pool
class Buffer final {
  public:
    static constexpr uint32_t InvalidBlockId = 0;

    friend class BufferManager;

  private:
    // Id for BufferCtl block
    uint32_t _blockId;

    // Get index for associated entry in buffer cache
    uint32_t GetIndex() {
        assert(this->IsValid());
        return this->_blockId - 1;
    }

    CacheEntry *getCacheEntry();
    const CacheEntry *getCacheEntry() const;
  public:
    Buffer();
    Buffer(uint32_t blockId);

    std::byte *GetContents();
    const std::byte *GetContents() const;
    void Lock(bool shared);
    void Unlock(bool shared);

    void MarkDirty();
    bool IsDirty();

    bool IsValid() const { return this->_blockId != InvalidBlockId; }

    static Buffer Invalid() { return Buffer{InvalidBlockId}; }

    void lock() {
      this->Lock(false);
    }
    void lock_shared() {
      this->Lock(true);
    }
    void unlock() {
      this->Unlock(false);
    }
    void unlock_shared() {
      this->Unlock(true);
    }
};
}; // namespace mi::storage::buffer
