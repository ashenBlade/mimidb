#pragma once

#include "adt/HashTable.hpp"
#include "executor/Oid.hpp"
#include "lock/LWLatch.hpp"
#include "storage/buffer/Buffer.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageTag.hpp"
#include "storage/buffer/internal/BufferData.hpp"
#include "storage/buffer/internal/CacheEntry.hpp"
#include <atomic>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace mi::storage::buffer {

class CacheEntryList {
  private:
    CacheEntry *_head;
    CacheEntry *_tail;
    uint32_t _size;

  public:
    // Latch to protect contents
    lock::LWLatch Latch;

    struct ReverseCacheEntryListIterator {
        CacheEntry *Current;
        ReverseCacheEntryListIterator &operator++() {
            if (Current) {
                Current = Current->Prev;
            }
            return *this;
        }
        bool operator==(const ReverseCacheEntryListIterator &other) {
            return this->Current == other.Current;
        }
        
        bool operator!=(const ReverseCacheEntryListIterator &other) {
            return !(*this == other);
        }

        CacheEntry *operator->() {
          return this->Current;
        }
        CacheEntry *operator&() {
          return this->Current;
        }
    };

    using reverse_iterator = ReverseCacheEntryListIterator;

  public:
    CacheEntryList();

    // Remove tail entry from list.
    // Can return NULL if list is empty.
    CacheEntry *PopTail();
    // Insert given entry into MRU of list.
    void InsertMRU(CacheEntry *entry);
    // Remove given entry from list
    void RemoveEntry(CacheEntry *entry);
    // Move entry from list to
    void MoveMRU(CacheEntry *entry);
    // Get size of list
    uint32_t Size();

    std::unique_lock<lock::LWLatch> Lock() { return std::unique_lock{this->Latch}; }

    std::shared_lock<lock::LWLatch> LockShared() { return std::shared_lock{this->Latch}; }

    // Iteration
    reverse_iterator rbegin() {
      return reverse_iterator{this->_tail};
    }
    reverse_iterator rend() {
      return reverse_iterator{nullptr};
    }
};

class BufferManager {
    // Allow Buffer to access buffer array
    friend class Buffer;

  private:
    adt::HashTable<PageTag, uint32_t, PageTagHash> _map;

    // Total amount of pages
    uint32_t _npages;

    // Descriptors for cache entries: both buffers and ghosts
    std::vector<CacheEntry> _entries;
    // All allocated BufferData structures
    std::vector<BufferData> _buffers;

    // List of free buffers.
    lock::LWLatch _freeBuffersLock;
    std::list<BufferData *> _freeBuffers;

    // Lists for ARC

    // Top recency
    CacheEntryList T1;
    // Top frequency
    CacheEntryList T2;
    // Bottom recency
    CacheEntryList B1;
    // Bottom frequency
    CacheEntryList B2;
    // List of free entries (not used)
    CacheEntryList FreeList;

    // Target T1 size (adaptive parameter)
    std::atomic_uint32_t P;

    // REPLACE function from paper.
    void adjust(uint32_t p, bool fromB2);

    // Find free or release existing (from B list) cache entry.
    // Returned entry is cleared and ready to be used.
    // Can return null if no free entry found.
    CacheEntry *evictCacheEntry();

    // Handle case when entry is hit in B list - 'isFrequency' tells which one.
    void handleGhostHit(CacheEntry *entry, bool isFrequency);

    // Handle case when entry is hit in T list - 'isFrequency' tells which one.
    void handleCacheHit(CacheEntry *entry);

    // Move valid T cache entry into B with possible buffer flush.
    // isFrequency tells which B list to add in. We should keep this flag, because
    // before we pass entry to this function it can change it's list, so our
    // operation will be invalid.
    bool tryVacateEntry(CacheEntry *entry, bool isFrequency);

    // Search for one entry in given B list to remove it, respecting it's pin count,
    // and removing it from map.
    // NOTE: list is locked inside, so pass unlocked
    void forgetAnyEntry(CacheEntryList *list);

    // When we want to insert new entry, then we must make sure we have enough
    // free space in lists. This subroutine is a prologue that must be executed
    // before we find find any free entry which finds entry from T and moves it
    // to B list freeing it's buffer.
    // This allows us to create/find new unused cache entry 
    void preinsertCleanup();

    // Utility function to create BufferPin object from it's cache entry.
    // Used when returning BufferPin to user.
    BufferPin makeBufferPin(CacheEntry *entry);

  public:
    BufferManager(uint32_t npages);

    /// @brief Get buffer for given page number
    /// @param relid Id of relation to get page
    /// @param pageno Number of page to get
    /// @return Buffer entry to get access to buffer and it's contents
    BufferPin GetBuffer(PageTag tag);

    /// @brief Return buffer to pool
    /// @param buffer Buffer to return
    /// @param tag Identifier of page
    void ReturnBuffer(Buffer buffer);

    /// @brief Add 1 new page to relation at the end
    /// @return Number of newly allocated and written page
    BufferPin ExtendRelation(Oid relid);
};
}; // namespace mi::storage::buffer
