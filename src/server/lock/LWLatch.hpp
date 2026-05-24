#pragma once

#include "lock/LockList.hpp"
#include "lock/LockMode.hpp"
#include <atomic>

namespace mi::lock {

class LWLatch {
  private:
    /// @brief Atomic state for latch
    std::atomic_uint32_t _state;
    /// @brief List of waiters for this lock
    LockListHead _waiters;

    void queueSelf(LockMode mode);
    void dequeueSelf();
    bool attemptLock(LockMode mode);
    void wakeup();
  public:
    LWLatch();

    LWLatch(const LWLatch &other) = delete;
    LWLatch &operator=(const LWLatch &other) = delete;

    void Lock(LockMode mode);
    void Unlock(LockMode mode);

    // This part is only to support unique_lock and shared_lock
    void lock() {
      this->Lock(LockMode::Exclusive);
    }
    void unlock() {
      this->Unlock(LockMode::Exclusive);
    }
    void lock_shared()  {
      this->Lock(LockMode::Share);
    }
    void unlock_shared() {
      this->Unlock(LockMode::Share);
    }
    
    ~LWLatch();
};

}; // namespace mi::lock
