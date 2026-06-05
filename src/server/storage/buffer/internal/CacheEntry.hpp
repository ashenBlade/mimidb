#pragma once

#include "storage/buffer/PageTag.hpp"
#include "storage/buffer/internal/BufferData.hpp"
#include <atomic>

namespace mi::storage::buffer {

// State flags for CacheEntry
enum CacheEntryFlags : int32_t {
    // Entry is locked due to concurrent changes in it, i.e. it is assigned to
    // another list
    Locked = (1 << 31),

    // PageTag field is valid. If unset it means that entry must be in FreeList.
    TagValid = (1 << 30),

    // The following 2 bits it combination tell to which list this entry belongs.
    // First bit tells if this is a Top list, the second if this is frequency.
    // So, rough value mapping is the following:
    // 00 - B1
    // 01 - B2
    // 10 - T1
    // 11 - T2
    IsTopList = (1 << 29),
    IsFrequencyList = (1 << 28),

    // Read or write is in progress
    IoInProgress = (1 << 27),

    // Previous IO (read/write) failed
    IoError = (1 << 26),

    // Stored data in page is valid
    DataValid = (1 << 25),

    // Page data is dirty
    Dirty = (1 << 26),
};

// Mask to get only list information from buffer state
inline constexpr int32_t BufferCtlListMask =
    CacheEntryFlags::IsTopList | CacheEntryFlags::IsFrequencyList;
// Mask to get reference count from state
inline constexpr int32_t BufferRefCountMask = (1 << 20) - 1;
// Number to use during ref count increment/decrement
inline constexpr int32_t BufferRefCountOne = 1;

// Entry in buffer cache
class CacheEntry {
  private:
    int32_t WaitUnlocked();
    void WaitIO();

  public:
    // Page tag for entry
    PageTag Tag;
    // Global index in cache
    uint32_t BlockId;
    // State for this entry
    std::atomic_int32_t State;
    // Assigned BufferData or nullptr if not assigned
    std::atomic<BufferData *> Buffer;

    // Pointer to next entry in list
    CacheEntry *Prev;
    // Pointer to prev entry in list
    CacheEntry *Next;

    // Rouge cache entry is an entry assigned to some list, but removed
    // from it, so it's pointers are nullified. This is temporary state
    // that happens during transitioning between different lists.
    bool IsRouge() const { return this->Prev == nullptr && this->Next == nullptr; }

    int32_t Lock();
    void Unlock();

    void Pin();
    void Unpin();

    // Reset state. Used when not used anymore and is moving to free list
    void TearDown();

    // Flush buffer to disk if dirty
    void FlushBuffer();

    // Read buffer from disk. Note that buffer must already be assigned.
    void ReadBuffer();

    bool ShouldStartIO(bool forRead);
    void TerminateBufferIO(int32_t setFlags, int32_t unsetFlags);
};

} // namespace mi::storage::buffer
