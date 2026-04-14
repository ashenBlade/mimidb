#include "mimidb.hpp"

#include "lock/Barrier.hpp"
#include "storage/Buffer.hpp"
#include "storage/BufferLock.hpp"

using namespace mi::storage;

BufferLock::BufferLock(std::shared_ptr<Buffer> buffer): _buffer(nullptr), _locked(false) {
    buffer->Lock(false);
    _buffer = buffer;
    lock::Barrier::Write();
    _locked = true;
}

BufferLock::BufferLock(): _buffer(nullptr), _locked(false) {};

BufferLock::BufferLock(BufferLock &&other) {
    if (this == &other) {
        return;
    }

    // release lock if there is one
    if (this->_locked) {
        this->_buffer->Unlock(false);
        this->_locked = false;
    }

    std::swap(this->_buffer, other._buffer);
    std::swap(this->_locked, other._locked);
}

BufferLock &BufferLock::operator= (BufferLock &&other) {
    if (this == &other) {
        return *this;
    }

    // release lock if there is one
    if (this->_locked) {
        this->_buffer->Unlock(false);
        lock::Barrier::Write();
        this->_locked = false;
    }

    std::swap(this->_buffer, other._buffer);
    std::swap(this->_locked, other._locked);
    return *this;
}

void BufferLock::Release() {
    if (!this->_locked) {
        return;
    }

    this->_buffer->Unlock(false);
    lock::Barrier::Write();
    this->_locked = false;
}

void BufferLock::Lock() {
    if (this->_locked) {
        return;
    }

    this->_buffer->Lock(false);
    lock::Barrier::Write();
    this->_locked = true;
}

BufferLock::~BufferLock() {
    if (!this->_locked) {
        return;
    }

    this->_buffer->Unlock(false);
    this->_locked = false;
}
