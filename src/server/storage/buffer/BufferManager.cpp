#include "storage/buffer/BufferManager.hpp"
#include "executor/Oid.hpp"
#include "lock/LWLatch.hpp"
#include "mi_config.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageTag.hpp"
#include "storage/buffer/RelFile.hpp"
#include "storage/buffer/internal/CacheEntry.hpp"
#include <assert.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace mi::storage::buffer;

void BufferManager::adjust(uint32_t p, bool fromB2) {
    auto ls = this->T1.LockShared();
    auto t1size = this->T1.Size();
    bool isFrequency;
    CacheEntryList *list;

    if (t1size > 0 && (t1size > p || (t1size == p && fromB2))) {
        ls.unlock();
        list = &this->T1;
        isFrequency = false;
    } else {
        ls.unlock();
        list = &this->T2;
        isFrequency = true;
    }

    // We decided which list to adjust
    auto l = list->Lock();
    for (auto it = list->rbegin(); it != list->rend(); ++it) {
        auto entry = &it;
        if (this->tryVacateEntry(entry, isFrequency)) {
            break;
        }
    }
}

CacheEntryList::CacheEntryList() : _head(nullptr), _tail(nullptr), _size(0), Latch() {};

uint32_t CacheEntryList::Size() { return this->_size; }

CacheEntry *CacheEntryList::PopTail() {
    for (auto entry = this->_tail; entry != nullptr; entry = entry->Prev) {
        auto state = entry->Lock();
        if ((state & BufferRefCountMask) != 0) {
            entry->Unlock();
            continue;
        }

        this->RemoveEntry(entry);
        entry->Unlock();

        --this->_size;

        return entry;
    }

    return nullptr;
}

void CacheEntryList::InsertMRU(CacheEntry *entry) {
    // When inserting to list entry must not belong to another list
    assert(entry->IsRouge());

    if (this->_size == 0) {
        this->_head = this->_tail = entry;
    } else {
        this->_head->Prev = entry;
        entry->Next = this->_head;
        this->_head = entry;
    }

    ++this->_size;
}

void CacheEntryList::RemoveEntry(CacheEntry *entry) {
    assert(this->_size > 0);
    if (this->_size == 0) {
        throw std::runtime_error("can not remove entry from list: list is empty");
    }

    if (this->_size == 1) {
        // If there is only 1 element, then passed entry must be rough, since
        // it can not point to any other element in list and also it must be
        // head and tail of list
        assert(entry->IsRouge());
        assert(this->_head == this->_tail && this->_tail == entry);

        this->_head = this->_tail = nullptr;
    } else {
        // If there are more than 1 entry is list, then entry can not be rouge,
        // because it must point to at list 1 entry.
        assert(!entry->IsRouge());
        if (entry->Next == nullptr) {
            assert(this->_tail == entry);
        } else if (entry->Prev == nullptr) {
            assert(this->_head == entry);
        }

        auto prev = entry->Prev;
        auto next = entry->Next;
        if (prev) {
            prev->Next = entry->Next;
        }

        if (next) {
            next->Prev = entry->Prev;
        }

        if (this->_head == entry) {
            this->_head = next;
        } else if (this->_tail == entry) {
            this->_tail = prev;
        }
    }

    entry->Prev = entry->Next = nullptr;
    --this->_size;
}

void CacheEntryList::MoveMRU(CacheEntry *entry) {
    assert(!entry->IsRouge());
    assert(this->_size > 0);

    if (this->_size == 1) {
        // Nothing to do
        return;
    }

    this->RemoveEntry(entry);
    this->InsertMRU(entry);
}

CacheEntry *BufferManager::evictCacheEntry() {
    // First, try to find free cache entry in free list if any
    {
        auto l = this->FreeList.Lock();
        auto x = this->FreeList.PopTail();
        if (x) {
            x->TearDown();
            return x;
        }
    }

    // Entry not found in free list - try to evict it from B1 or B2.
    // Start from B1, because B2 is more likely to be used in the near future.
    //
    // NOTE: do not search in T lists, because they are limited in size, and
    // can not exceed _npages (while all cache entries count is _npages * 2),
    // so all possible free entries are located in free and B lists.
    for (auto list : {&this->B1, &this->B2}) {
        // Lock list before iteration
        auto listLock = list->Lock();
        for (auto it = list->rbegin(); it != list->rend(); ++it) {
            auto entry = &it;
            auto state = entry->Lock();
            entry->Unlock();
            if ((state & BufferRefCountMask) != 0) {
                // Entry is pinned
                continue;
            }

            // Lock map to remove this entry
            auto mapLock = this->_map.LockPartition(entry->Tag);

            // Check entry is not pinned once more
            state = entry->State.load();
            if ((state & BufferRefCountMask) != 0) {
                // Someone pinned entry - skip
                continue;
            }

            if (!this->_map.Remove(entry->Tag)) {
                throw std::runtime_error(
                    "buffer page map is broken - entry is not presented in list");
            }

            // Completely remove entry from list and then release all locks.
            list->RemoveEntry(entry);
            mapLock.unlock();
            listLock.unlock();

            // Entry not in any list and accessed only by us.
            // Tear it down and return as completely new entry.
            entry->TearDown();
            return entry;
        }
    }

    // Nothing found
    return nullptr;
}

void BufferManager::forgetAnyEntry(CacheEntryList *list) {
    auto listLock = list->Lock();
    for (auto it = list->rbegin(); it != list->rend(); ++it) {
        auto entry = &it;
        auto state = entry->State.load();
        if ((state & BufferRefCountMask) != 0) {
            continue;
        }

        auto mapLock = this->_map.LockPartition(entry->Tag);
        state = entry->State.load();
        if ((state & BufferRefCountMask) != 0) {
            continue;
        }

        // Cache entry is not pinned by anyone - feel free to remove
        if (!this->_map.Remove(entry->Tag)) {
            throw std::runtime_error(
                "page table is broken - cache entry must be in map but absent");
        }

        // We do not need map lock anymore
        mapLock.unlock();

        // Entry is removed, now just clean it and move to free list
        listLock = this->FreeList.Lock();
        this->FreeList.InsertMRU(entry);
        // all things done
        return;
    }
}

void BufferManager::preinsertCleanup() {
    uint32_t t1size;
    uint32_t b1size;
    {
        auto l1 = this->B1.LockShared();
        auto l2 = this->T1.LockShared();
        t1size = this->T1.Size();
        b1size = this->B1.Size();
    }

    if (t1size + b1size >= this->_npages) {
        if (t1size < this->_npages) {
            this->forgetAnyEntry(&this->B1);
            this->adjust(this->P, false);
        } else {
            auto l = this->T1.Lock();
            for (auto it = this->T1.rbegin(); it != this->T1.rend(); ++it) {
                auto entry = &it;
                if (this->tryVacateEntry(entry, false)) {
                    break;
                }
            }
        }
    } else if (t1size + b1size < this->_npages) {
        uint32_t t2size;
        uint32_t b2size;
        {
            auto l1 = this->B2.LockShared();
            auto l2 = this->T2.LockShared();
            t2size = this->T2.Size();
            b2size = this->B2.Size();
        }

        auto sum = t1size + b1size + t2size + b2size;
        if (sum >= this->_npages) {
            if (sum == this->_npages * 2) {
                this->forgetAnyEntry(&this->B2);
            }

            this->adjust(this->P, false);
        }
    }
}

void BufferManager::handleCacheHit(CacheEntry *entry) {
    // This entry already in top list. Now this buffer is referenced at least
    // twice, so move it to MRU in T2.

    // Take lock before modifications, otherwise we can spoil different
    // lists (concurrent remove/insert across different lists).
    auto state = entry->State.load();
    
    // Verify that this entry belongs to top list. Since entry must be pinned
    // at this point, then it can not move to B list, but anyway check.
    assert(state & CacheEntryFlags::IsTopList);

    if (state & CacheEntryFlags::IsFrequencyList) {
        // Entry is in T2 list - just move it to MRU position
        auto l = this->T2.Lock();

        state = entry->Lock();

        // We can not move from T2 to T1, by design but anyway check that we 
        // still in T2.
        assert ((state & CacheEntryListMask) == (CacheEntryFlags::IsTopList | CacheEntryFlags::IsFrequencyList));
        this->T2.MoveMRU(entry);
        entry->Unlock();
    } else {
        auto l1 = this->T1.Lock();
        auto l2 = this->T2.Lock();
        auto state = entry->Lock();
        if ((state & CacheEntryListMask) != CacheEntryFlags::IsFrequencyList) {
            // Entry in T1 list - move to T2
            this->T1.RemoveEntry(entry);
            this->T2.InsertMRU(entry);
            entry->State.fetch_or(CacheEntryFlags::IsFrequencyList);
        } else {
            // Entry was concurrently moved to T2, so just move to MRU
            this->T2.MoveMRU(entry);
        }
        entry->Unlock();
    }
}

// Here we handle cache miss cases: II and III.
// Passed cache entry must be already locked.
void BufferManager::handleGhostHit(CacheEntry *entry, bool isFrequency) {
    // First we must perform parameter adaptation
    uint32_t b1size;
    uint32_t b2size;
    {
        auto l1 = std::shared_lock{this->B1.Latch};
        auto l2 = std::shared_lock{this->B2.Latch};
        b1size = this->B1.Size();
        b2size = this->B2.Size();
    }

    uint32_t delta;
    auto p = this->P.load();
    auto newP = p;
    if (isFrequency) {
        delta = b1size >= b2size ? 1 : b2size / b1size;
        newP = std::min(p + delta, this->_npages);
    } else {
        delta = b2size >= b1size ? 1 : b1size / b2size;
        newP = std::max(p - delta, 0U);
    }

    // In concurrent environment we never can achieve exact value, so try once
    // to adapt value, because otherwise we must constantly get S lock on both
    // lists and try again which limits concurrency
    this->P.compare_exchange_strong(p, newP);
    this->adjust(p, isFrequency);

    // Now actually move entry to T2
    int32_t oldListMask;
    CacheEntryList *oldList;
    if (isFrequency) {
        oldList = &this->B1;
        oldListMask = CacheEntryFlags::IsFrequencyList;
    } else {
        oldList = &this->B2;
        oldListMask = 0;
    }

    {
        auto l1 = oldList->Lock();
        auto l2 = this->T2.Lock();

        auto state = entry->Lock();
        if ((state & CacheEntryListMask) == oldListMask) {
            oldList->RemoveEntry(entry);
            this->T2.InsertMRU(entry);
            entry->State.fetch_or(CacheEntryFlags::IsTopList | CacheEntryFlags::IsFrequencyList);
        }
        entry->Unlock();
    }
}

BufferPin BufferManager::makeBufferPin(CacheEntry *entry) {
    // Returned buffer must be valid and ready to use
    assert(entry->State.load() & CacheEntryFlags::DataValid);
    assert((entry->State.load() & BufferRefCountMask) > 0);
    assert(entry->Buffer.load() != nullptr);

    return BufferPin{entry->Tag, Buffer{this, entry->BlockId + 1}};
}

bool BufferManager::tryVacateEntry(CacheEntry *entry, bool isFrequency) {
    auto state = entry->Lock();
    if ((state & BufferRefCountMask) > 0) {
        // Someone concurrently pinned this entry, so it is used again
        entry->Unlock();
        return false;
    }

    // Before flushing buffer to disk we must pin it, otherwise concurent worker
    // may also select this entry to vacate
    entry->Pin();
    entry->Unlock();

    // Actually flush buffer to disk
    entry->FlushBuffer();

    CacheEntryList *list;
    if (isFrequency) {
        list = &this->B2;
    } else {
        list = &this->B1;
    }

    // After we have done flushing verify no one else pinned entry.
    // Do it before taking lock on list, in order not to waste time.
    state = entry->Lock();
    entry->Unlock();
    if ((state & BufferRefCountMask) != 1) {
        return false;
    }

    // Lock target list now - do not
    auto listLock = list->Lock();

    // Now we are ready to make ghost entry
    state = entry->Lock();
    if ((state & BufferRefCountMask) != 1) {
        // Someone pinned buffer
        entry->Unlock();
        return false;
    }

    // No-one still hold pin except us.

    // Extract buffer object to later add it to free list
    auto buffer = entry->Buffer.load();
    entry->Buffer = nullptr;

    entry->Unpin();

    // Update list information of this entry: remove from Top list (to B) and set R/F sublist
    // accordingly
    entry->State.fetch_and(~CacheEntryFlags::IsTopList);
    if (isFrequency) {
        entry->State.fetch_or(CacheEntryFlags::IsFrequencyList);
    } else {
        entry->State.fetch_and(~CacheEntryFlags::IsFrequencyList);
    }

    // Actually insert to target list
    list->InsertMRU(entry);

    // All modifications are done - unlock entry
    entry->Unlock();
    listLock.unlock();

    // Finally, move freed buffer to free list.
    // Note that buffer can be null, because cache entry can be assigned to
    // failed relation extend operation, when buffer was not assigned.
    if (buffer != nullptr) {
        auto l2 = std::unique_lock{this->_freeBuffersLock};
        this->_freeBuffers.push_back(buffer);
    }

    return true;
}


BufferPin BufferManager::GetBuffer(PageTag tag) {
    // At first we must find/obtain id for given entry
    auto partLockS = this->_map.LockPartitionShared(tag);
    auto blkId = this->_map.Get(tag);

    // Allocated and pinned entry
    CacheEntry *entry;
    // Flag indicating that entry was created now, so needs to process hit
    bool created;
    if (blkId == nullptr) {
        // Cache miss.
        partLockS.unlock();

        entry = this->evictCacheEntry();
        if (entry != nullptr) {
            // Before inserting entry into hash table setup cache entry state appropriately.
            entry->Tag = tag;
            entry->State.store(CacheEntryFlags::TagValid);
            assert(entry->Buffer == nullptr);

            auto mapLockX = this->_map.LockPartition(tag);
            auto [existing, inserted] = this->_map.Insert(tag, entry->BlockId);
            if (inserted) {
                // We have successfully inserted entry into the map.
                // Now insert it into T1 list

                // We can not hold lock on entry, because we are holding X lock on map
                entry->State.fetch_or(CacheEntryFlags::IsTopList);
                entry->Pin();

                // It's bad to acquire list lock while locking map, but for now
                // it hard to handle rouge entries while not in lock.
                {
                    auto l = this->T1.Lock();
                    this->T1.InsertMRU(entry);
                }

                mapLockX.unlock();

                created = true;
            } else {
                // Someone inserted entry for the same tag concurrently.
                // Move our cache entry to free list.
                auto oldEntry = entry;

                entry = &this->_entries[*existing];

                // Pin entry
                entry->Pin();

                mapLockX.unlock();

                // Move obtained cache entry to free list as soon as possible
                {
                    auto l = this->FreeList.Lock();
                    this->FreeList.InsertMRU(oldEntry);
                }

                created = false;
            }
        } else {
            // No free cache entry found, but maybe other workers invoked us
            // with the same page tag and grabbed last free cache entry.
            // Check if entry with same tag exists.
            partLockS = this->_map.LockPartitionShared(tag);

            blkId = this->_map.Get(tag);
            if (blkId == nullptr) {
                throw std::runtime_error("no free cache entries found");
            }

            entry = &this->_entries[*blkId];
            entry->Pin();

            created = false;
        }
    } else {
        // Entry is found in cache: either alive or ghost.
        auto entryId = *blkId;
        entry = &this->_entries[entryId];

        // Pin entry before unlocking map lock, so no one will evict entry from cache.
        // Also, pin ensures that no-one will evict this entry
        entry->Pin();
        partLockS.unlock();

        created = false;
    }

    // If we actually made a cache/ghost hit, then process this.
    if (!created) {
        auto state = entry->State.load();
        if (state & CacheEntryFlags::IsTopList) {
            this->handleCacheHit(entry);
        } else {
            this->handleGhostHit(entry, state & CacheEntryFlags::IsFrequencyList);
        }
    }

    // Up to this moment we have entry moved to top list and pinned, but no lock
    // is hold.

    // Now check that buffer is assigned and valid.

    // Very likely that everything already setup
    if (entry->State.load() & CacheEntryFlags::DataValid) {
        assert(entry->Buffer.load() != nullptr);
        return this->makeBufferPin(entry);
    }

    // Buffer is not yet setup.

    // Check we have buffer page assigned
    if (entry->Buffer.load() == nullptr) {
        // Search free buffer in list
        auto lock = std::unique_lock{this->_freeBuffersLock};
        if (this->_freeBuffers.size() > 0) {
            auto buffer = this->_freeBuffers.back();
            this->_freeBuffers.pop_back();
            lock.unlock();

            auto old = entry->Buffer.load();
            if (!(old == nullptr && entry->Buffer.compare_exchange_strong(old, buffer))) {
                // Another worker can concurrently find free buffer data.
                // If we are here, then this happened and buffer is set.
                // We must return our obtained buffer back.
                lock.lock();
                this->_freeBuffers.push_back(buffer);
                lock.unlock();
            }
        } else if (entry->Buffer.load() == nullptr) {
            // No free buffers, but we checked this after got X lock on list,
            // so someone can already get buffer and assign it.
            // But if we are in this branch, then that not true and we really
            // failed to find free page.
            //
            // TODO: consider moving entry to B
            throw std::runtime_error("no free buffers");
        }

        assert(entry->Buffer.load() != nullptr);
    }

    // Now verify page data is valid: read from file and validated.
    if (!(entry->State.load() & CacheEntryFlags::DataValid)) {
        entry->ReadBuffer();
        assert(entry->State.load() & CacheEntryFlags::DataValid);
    }

    return this->makeBufferPin(entry);
}

void BufferManager::ReturnBuffer(Buffer buffer) {
    assert(buffer.IsValid());

    auto entry = &this->_entries[buffer.GetIndex()];

    // If we are unpinning entry, then we must
    assert((entry->State.load() & BufferRefCountMask) != 0);

    entry->Unpin();
}

BufferPin BufferManager::ExtendRelation(Oid relid) {
    assert(relid.IsValid());

    // We can look at relation extension the same as cache miss, because the
    // page wasn't used by anyone: perform preinsert cleanup (so we have free
    // cache entry and possibly buffer) and actually evict some entry.
    this->preinsertCleanup();
    auto entry = this->evictCacheEntry();
    if (entry == nullptr) {
        throw std::runtime_error("no free cache entry found");
    }

    auto relfile = RelFile::Open(relid, O_RDWR);
    auto nblocks = relfile.GetPagesCount();

    auto tag = PageTag{relid, nblocks};

    auto mapLock = this->_map.LockPartition(tag);
    auto [existing, inserted] = this->_map.Insert(tag, entry->BlockId);
    if (inserted) {
        // Entry state must be clear
        constexpr auto emptyEntryFlags =
            CacheEntryFlags::DataValid | CacheEntryFlags::TagValid | CacheEntryFlags::Dirty;
        assert(!(entry->State.load() & emptyEntryFlags));

        // initialize tag
        entry->Tag = tag;
        entry->State.fetch_or(CacheEntryFlags::TagValid);
        // pin entry
        entry->Pin();

        // Insert new entry to T1 list, but still hold map lock
        //
        // NOTE: this is bad, because we are grabbing T1 lock inside map lock
        // which limits concurency, but i am too lazy to fix concurrency here,
        // so leave inner lock as is.
        {
            auto l = this->T1.Lock();
            this->T1.InsertMRU(entry);
            entry->State.fetch_or(CacheEntryFlags::IsTopList);
        }

        mapLock.unlock();

        // Only 1 worker can invoke extend, so this call must succeed
        auto shouldStart = entry->ShouldStartIO(true);
        assert(shouldStart);
    } else {
        // This can happen, because this is not the first attempt to extend relation
        // and previous call failed without setting DataValid bit.

        auto oldEntry = entry;
        entry = &this->_entries[*existing];

        // Pin entry before unlocking hash table
        entry->Pin();
        mapLock.unlock();

        // Return obtained cache entry and work with existing
        {
            auto l = this->FreeList.Lock();
            this->FreeList.InsertMRU(oldEntry);
        }

        auto state = entry->State.load();
        if (!(state & CacheEntryFlags::IsTopList)) {
            // It is possible that our cache entry moved from T to B.
            // We invoke handleGhostHit, not because it's valid (it is not because
            // entry logically is still not used by anyone), but because it has
            // required logic with releasing unused cache entries and moving
            // our entry to Top list.
            this->handleGhostHit(entry, state & CacheEntryFlags::IsFrequencyList);
        }

        do {
            entry->Lock();
            entry->State.fetch_and(~CacheEntryFlags::DataValid);
            entry->Unlock();
            // idk why use read instead of write, but this is how it's done in pg
        } while (!entry->ShouldStartIO(true));
    }

    // Verify that we have buffer assigned
    entry->Lock();
    if (entry->Buffer == nullptr) {
        entry->Unlock();
        auto l = std::unique_lock{this->_freeBuffersLock};
        entry->Lock();
        if (entry->Buffer == nullptr) {
            if (this->_freeBuffers.size()) {
                auto buffer = this->_freeBuffers.back();
                this->_freeBuffers.pop_back();
                entry->Buffer = buffer;
            } else {
                entry->Unlock();
                throw std::runtime_error("no free buffer found");
            }
        } else {
            // Someone concurrently assigned buffer - nothing to do
        }
    }
    assert(entry->Buffer != nullptr);
    entry->Unlock();

    // Take a lock for IO
    auto lock = std::unique_lock{entry->Buffer.load()->Latch};

    // Now we are holding lock and must perform IO
    try {
        // Actually extend relation
        relfile.Extend(nblocks);

        // Write all zeros to allocated buffer
        std::memset(entry->Buffer.load()->Page.get(), 0, Config::PageSize);

        // Do not call abort - next caller will make another attempt
        entry->TerminateBufferIO(CacheEntryFlags::DataValid, 0);
    } catch (...) {
        entry->TerminateBufferIO(CacheEntryFlags::IoError, 0);
        throw;
    }

    return this->makeBufferPin(entry);
}

BufferManager::BufferManager(uint32_t npages)
    : _map(8), _npages(npages), _freeBuffersLock(), _freeBuffers() {

    if (UINT32_MAX <= static_cast<size_t>(_npages) * 2) {
        throw std::runtime_error("too many blocks in buffer");
    }

    // Initialize cache
    _entries = std::vector<CacheEntry>{_npages * 2};

    for (uint32_t i = 0; i < _entries.size(); i++) {
        auto entry = &_entries[i];

        // Assign their block id
        entry->BlockId = i;

        // Init all fields
        entry->State.store(0);
        entry->Tag = {};
        entry->Next = entry->Prev = nullptr;

        // All cache entries are initially free
        this->FreeList.InsertMRU(entry);
    }

    // Initialize buffer data
    _buffers = std::vector<BufferData>{_npages};
    for (size_t i = 0; i < _npages; i++) {
        // Allocate page data
        _buffers[i].Page = std::make_unique<std::byte[]>(Config::PageSize);

        // All buffers initially are free
        this->_freeBuffers.push_back(&_buffers[i]);
    }
}
