#pragma once

#include <memory>

#include "lock/Barrier.hpp"
#include "storage/buffer/Buffer.hpp"

namespace mi::storage {

template <bool VShared> class BufferLockBase {
  private:
    std::shared_ptr<Buffer> _buffer;
    bool _locked;

  public:
    BufferLockBase() : _buffer(nullptr), _locked(false) {}
    BufferLockBase(std::shared_ptr<Buffer> buffer) : _buffer(nullptr), _locked(false) {
        buffer->Lock(VShared);
        _buffer = buffer;
        lock::Barrier::Write();
        _locked = true;
    }

    std::shared_ptr<Buffer> GetBuffer();
    const std::shared_ptr<Buffer> GetBuffer() const;

    BufferLockBase(BufferLockBase &&other) {
        if (this == &other) {
            return;
        }

        // release lock if there is one
        if (this->_locked) {
            this->_buffer->Unlock(VShared);
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
            this->_buffer->Unlock(VShared);
            lock::Barrier::Write();
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

        this->_buffer->Unlock(VShared);
        lock::Barrier::Write();
        this->_locked = false;
    }
    void Lock() {
        if (this->_locked) {
            return;
        }

        this->_buffer->Lock(VShared);
        lock::Barrier::Write();
        this->_locked = true;
    }

    ~BufferLockBase() {
        if (!this->_locked) {
            return;
        }

        this->_buffer->Unlock(VShared);
        this->_locked = false;
    }
};

// RAII wrapper for buffer lock in X mode
class BufferLock : public BufferLockBase<false> {};

// RAII wrapper for buffer lock in S mode
class BufferSharedLock : public BufferLockBase<true> {};

}; // namespace mi::storage
