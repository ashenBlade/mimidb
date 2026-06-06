#include "storage/buffer/Buffer.hpp"
#include "lock/LockMode.hpp"
#include "storage/buffer/BufferManager.hpp"

using namespace mi::storage::buffer;

Buffer::Buffer(): Buffer(nullptr, Buffer::InvalidBlockId) {};
Buffer::Buffer(BufferManager *pool, uint32_t blockId) : _pool(pool), _blockId(blockId) {};

CacheEntry *Buffer::getCacheEntry() {
    assert(this->IsValid());

    return &this->_pool->_entries[this->GetIndex()];
}

const CacheEntry *Buffer::getCacheEntry() const {
    assert(this->IsValid());

    return &this->_pool->_entries[this->GetIndex()];
}


std::byte *Buffer::GetContents() { 
    auto buffer = this->getCacheEntry()->Buffer;
    assert(buffer != nullptr);
    return buffer->Page.get();
}

const std::byte *Buffer::GetContents() const {
    auto buffer = this->getCacheEntry()->Buffer;
    assert(buffer != nullptr);
    return buffer->Page.get();
}

void Buffer::Lock(bool shared) {
    auto buffer = this->getCacheEntry()->Buffer;

    auto mode = shared ? lock::LockMode::Share : lock::LockMode::Exclusive;
    buffer->Latch.Lock(mode);
}

void Buffer::Unlock(bool shared) {
    auto buffer = this->getCacheEntry()->Buffer;

    auto mode = shared ? lock::LockMode::Share : lock::LockMode::Exclusive;
    buffer->Latch.Unlock(mode);
}

void Buffer::MarkDirty() {
    auto entry = this->getCacheEntry();
    entry->State.fetch_or(CacheEntryFlags::Dirty);
}
