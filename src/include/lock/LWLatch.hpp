#pragma once

#include "lock/LockMode.hpp"
#include "worker/WorkerList.hpp"

#include <atomic>

namespace mi::lock {

class LWLatch {
  private:
    /// @brief Internal state storing
    std::atomic_uint32_t _state;
    /// @brief Head of wait list
    mi::worker::WorkerListHead _waitList;

    /// @brief Make 1 attempt to acquire lock in specified mode
    /// @param mode Mode to obtain
    /// @return true if lock is obtained, otherwise false
    bool attemptLock(LockMode mode);
    /// @brief Queue self (worker) into waiting list
    /// @param mode Lock mode we are requesting
    void queueSelf(LockMode mode);
    /// @brief Remove self from waiting list
    void dequeueSelf();
    /// @brief Lock waitlist before performing modifications to it
    void waitListLock();
    /// @brief Unlock waitlist after performing modifications
    void waitListUnlock();
    /// @brief Wakeup all the lockers that currently have a chance to acquire the lock
    void wakeup();

  public:
    LWLatch();

    void Lock(LockMode mode);
    bool TryLock(LockMode mode);
    void Release(LockMode mode);
};

}; // namespace mi::lock
