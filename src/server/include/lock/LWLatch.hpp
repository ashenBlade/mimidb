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

    // TODO: тут возвращать какой-нибудь объект лока с само разлочиванием
    // XXX: а может переделать под mutex, чтобы автоматически с другими типами работал?
    void Lock(LockMode mode);
    void Unlock(LockMode mode);
    ~LWLatch();
};

}; // namespace mi::lock
