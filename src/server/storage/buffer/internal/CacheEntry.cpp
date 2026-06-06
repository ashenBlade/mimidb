#include "CacheEntry.hpp"
#include "lock/Spin.hpp"
#include "storage/buffer/RelFile.hpp"
#include <cassert>
#include <fcntl.h>
#include <mutex>

using namespace mi::storage::buffer;

int32_t CacheEntry::waitUnlocked() {
    auto state = this->State.load();
    while (state & CacheEntryFlags::Locked) {
        lock::Spin::PerformSpin();
        state = this->State.load();
    }

    return state;
}

void CacheEntry::Pin() {
    auto old = this->State.fetch_add(BufferRefCountOne);

    // Can not exceed max value which is mask itself
    assert((old & BufferRefCountMask) != BufferRefCountMask);
}

void CacheEntry::Unpin() {
    auto old = this->State.fetch_sub(BufferRefCountOne);

    // Must be pinned
    assert((old & BufferRefCountMask) > 0);
}

int32_t CacheEntry::Lock() {
    while (true) {
        auto old = this->State.fetch_or(CacheEntryFlags::Locked);
        if (!(old & CacheEntryFlags::Locked)) {
            return old | CacheEntryFlags::Locked;
        }

        this->waitUnlocked();
    }
}

void CacheEntry::Unlock() {
    auto prev = this->State.fetch_sub(CacheEntryFlags::Locked);

    // Must be locked previously
    assert(prev & CacheEntryFlags::Locked);
}

void CacheEntry::TearDown() {
    // Only history entries must go to free list
    assert(!(this->State.load() & CacheEntryFlags::IsTopList));

    // Noone must hold lock
    assert(!(this->State.load() & CacheEntryFlags::Locked));

    // Just clear to initial state
    this->State.store(0);

    // No need to clear tag, because TagValid flag is unset
    // this->Tag = {};
}

void CacheEntry::waitIO() {
    auto buffer = this->Buffer.load();
    assert(buffer != nullptr);
    while (true) {
        auto state = this->Lock();

        this->Unlock();

        if (!(state & CacheEntryFlags::IoInProgress)) {
            // No wait needed
            break;
        }

        // Вот эта часть некорректная (а может и ок)
        auto lock = std::unique_lock{buffer->Latch};
        state = this->State.load();
        if (state & CacheEntryFlags::IoInProgress) {
            buffer->IOCondVar.wait(lock);
        }
    }
}

void CacheEntry::TerminateBufferIO(int32_t setFlags, int32_t unsetFlags) {
    // These flags must not intersect
    assert((setFlags & unsetFlags) == 0);

    auto state = this->Lock();

    // We must be running IO right now
    assert(state & CacheEntryFlags::IoInProgress);

    // Clear IO related flags
    this->State.fetch_and(~(CacheEntryFlags::IoInProgress | CacheEntryFlags::IoError));

    this->State.fetch_and(~unsetFlags);
    this->State.fetch_or(setFlags);

    this->Unlock();

    auto buffer = this->Buffer.load();
    assert(buffer != nullptr);

    // Notify IO completed
    buffer->IOCondVar.notify_all();
}

bool CacheEntry::ShouldStartIO(bool forInput) {
    // Grab the lock (IoInProgress flag) first and only after it perform IO
    int32_t state;
    while (true) {
        state = this->Lock();

        if (!(state & CacheEntryFlags::IoInProgress)) {
            // No one has started IO - we are first
            break;
        }

        this->Unlock();
        this->waitIO();
    }

    if (forInput ? (state & CacheEntryFlags::DataValid) : !(state & CacheEntryFlags::Dirty)) {
        this->Unlock();
        return false;
    }

    this->State.fetch_or(CacheEntryFlags::IoInProgress);
    this->Unlock();

    return true;
}

void CacheEntry::ReadBuffer() {
    auto state = this->State.load();
    if (state & CacheEntryFlags::DataValid) {
        // Data is loaded successfully - nothing to do
        return;
    }

    if (!this->ShouldStartIO(true)) {
        // Someone successfully finished IO
        return;
    }

    // Buffer must be assigned
    auto buffer = this->Buffer.load();
    assert(buffer != nullptr);

    auto lock = std::unique_lock{buffer->Latch};
    try {
        auto file = RelFile::Open(this->Tag.Relid, O_RDONLY);
        file.Read(buffer->Page.get(), this->Tag.PageNo);
    } catch (...) {
        this->TerminateBufferIO(CacheEntryFlags::IoError, 0);
        throw;
    }

    this->TerminateBufferIO(CacheEntryFlags::DataValid, 0);
}

void CacheEntry::FlushBuffer() {
    if (!this->ShouldStartIO(false)) {
        // No IO needed
        return;
    }

    auto buffer = this->Buffer.load();
    assert(buffer != nullptr);

    auto l = std::unique_lock{buffer->Latch};

    // We grabbed the lock and can perform IO
    try {
        auto relfile = RelFile::Open(this->Tag.Relid, O_WRONLY);
        relfile.Write(buffer->Page.get(), this->Tag.PageNo);
        relfile.Fsync();
    } catch (...) {
        this->TerminateBufferIO(CacheEntryFlags::IoError, 0);
        throw;
    }

    this->TerminateBufferIO(CacheEntryFlags::DataValid, CacheEntryFlags::Dirty);
}
