#include "storage/buffer/Buffer.hpp"
#include "lock/LockMode.hpp"

using namespace mi::storage::buffer;

Buffer::Buffer(std::byte *contents) : _contents(contents), _latch(), _dirty(false) {};

std::byte *Buffer::GetContents() { return _contents; }

const std::byte *Buffer::GetContents() const { return _contents; }

void Buffer::Lock(bool shared) {
    auto mode = shared ? lock::LockMode::Share : lock::LockMode::Exclusive;
    this->_latch.Lock(mode);
}

void Buffer::Unlock(bool shared) {
    auto mode = shared ? lock::LockMode::Share : lock::LockMode::Exclusive;
    this->_latch.Release(mode);
}
