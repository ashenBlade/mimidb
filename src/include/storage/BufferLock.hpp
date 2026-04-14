#pragma once

#include <memory>

#include "storage/Buffer.hpp"

namespace mi::storage {
// RAII wrapper for buffer lock in X mode
class BufferLock {
  private:
    std::shared_ptr<Buffer> _buffer;
    bool _locked;

  public:
    BufferLock(std::shared_ptr<Buffer> buffer);
    BufferLock();

    BufferLock(BufferLock &&other);
    BufferLock &operator=(BufferLock &&other);

    BufferLock(const BufferLock &other) = delete;
    BufferLock &operator=(const BufferLock &other) = delete;
    
    void Release();
    void Lock();

    ~BufferLock();
};

// RAII wrapper for buffer lock in S mode
class BufferSharedLock {
  private:
    std::shared_ptr<Buffer> _buffer;
    bool _locked;
  public:
    BufferSharedLock(std::shared_ptr<Buffer> buffer);
    BufferSharedLock();

    BufferSharedLock(BufferLock &&other);
    BufferSharedLock &operator=(BufferSharedLock &&other);

    BufferSharedLock(const BufferSharedLock &other) = delete;
    BufferSharedLock &operator=(const BufferSharedLock &other) = delete;

    void Release();
    void LockShared();

    ~BufferSharedLock();
};

}; // namespace mi::storage
