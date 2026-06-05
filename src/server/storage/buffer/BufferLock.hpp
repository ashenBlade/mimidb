#pragma once

#include "lock/Barrier.hpp"
#include "storage/buffer/Buffer.hpp"

namespace mi::storage::buffer {

template <bool VShared> class BufferLockBase {
  private:
    Buffer _buffer;
    bool _locked;

  public:
    BufferLockBase() : _buffer(Buffer::Invalid()), _locked(false) {}
    BufferLockBase(Buffer buffer) : _buffer(Buffer::Invalid()), _locked(false) {
        buffer.Lock(VShared);
        _buffer = buffer;
        _locked = true;
    }

    BufferLockBase(BufferLockBase &&other) {
        if (this == &other) {
            return;
        }

        // release lock if there is one
        if (this->_locked) {
            this->_buffer.Unlock(VShared);
            this->_locked = false;
        }

        std::swap(this->_buffer, other._buffer);
        std::swap(this->_locked, other._locked);
    }
    BufferLockBase &operator=(BufferLockBase &&other) {
        if (this == &other) {
            return *this;
        }

        // release lock if there is one
        if (this->_locked) {
            this->_buffer.Unlock(VShared);
            this->_locked = false;
        }

        std::swap(this->_buffer, other._buffer);
        std::swap(this->_locked, other._locked);
        return *this;
    }

    BufferLockBase(const BufferLockBase &other) = delete;
    BufferLockBase &operator=(const BufferLockBase &other) = delete;

    void Release() {
        if (!this->_locked) {
            return;
        }

        this->_buffer.Unlock(VShared);
        this->_locked = false;
    }
    void Lock() {
        if (this->_locked) {
            return;
        }

        this->_buffer.Lock(VShared);
        this->_locked = true;
    }

    ~BufferLockBase() {
        if (!this->_locked) {
            return;
        }

        this->_buffer.Unlock(VShared);
        this->_locked = false;
    }
};

// RAII wrapper for buffer lock in X mode
class BufferLock : public BufferLockBase<false> {};

// RAII wrapper for buffer lock in S mode
class BufferSharedLock : public BufferLockBase<true> {};

}; // namespace mi::storage
