#include "storage/Buffer.hpp"
#include "storage/BufferManager.hpp"

using namespace mi::storage;

Buffer::Buffer(std::byte *contents): _contents(contents), _mutex(), _dirty(false) { };

std::byte *Buffer::GetContents() {
    return _contents;
}

const std::byte *Buffer::GetContents() const {
    return _contents;
}

void Buffer::Lock(bool shared) {
    if (shared)
        _mutex.lock_shared();
    else
        _mutex.lock();
}

void Buffer::Unlock(bool shared) {
    if (shared)
        _mutex.unlock_shared();
    else
        _mutex.unlock();
}
