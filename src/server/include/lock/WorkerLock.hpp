#pragma once

#include "lock/LockList.hpp"
#include "lock/LockMode.hpp"
#include "lock/WorkerLockState.hpp"
#include <semaphore>
namespace mi::lock {

// Information and state about locking for single worker
class WorkerLock {
  private:
    // Semaphore for locking (used by LWLatch)
    std::binary_semaphore _sema;
    // In which state current worker
    WorkerLockState _lockState;
    // Lock mode requested
    LockMode _lockMode;
    // Node of waiters list (if waiting for lock)
    LockListNode _lockNode;

  public:
    WorkerLock();

    std::binary_semaphore &GetSemaphore() { return this->_sema; }
    WorkerLockState GetLockState() const { return this->_lockState; }
    void SetLockState(WorkerLockState state) { this->_lockState = state; }
    LockMode GetLockMode() const { return this->_lockMode; }
    void SetLockMode(LockMode mode) { this->_lockMode = mode; }
    LockListNode &GetLockNode() { return this->_lockNode; }
};

} // namespace mi::lock
