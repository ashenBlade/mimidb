#include "mimidb.hpp"

#include "lock/LWLatch.hpp"
#include "lock/Spin.hpp"
#include "lock/Barrier.hpp"
#include "worker/Worker.hpp"
#include "worker_state.hpp"

using namespace mi::lock;
using namespace mi::worker;

#define LW_FLAG_HAS_WAITERS ((uint32_t)1 << 31)
#define LW_FLAG_RELEASE_OK ((uint32_t)1 << 30)
#define LW_FLAG_LOCKED ((uint32_t)1 << 29)
#define LW_FLAG_BITS 3
#define LW_FLAG_MASK (((1 << LW_FLAG_BITS) - 1) << (32 - LW_FLAG_BITS))

/* assumes MAX_BACKENDS is a (power of 2) - 1, checked below */
// TODO: remove pg specific
#define MAX_BACKENDS ((1U << 18) - 1)

#define LW_VAL_EXCLUSIVE (MAX_BACKENDS + 1)
#define LW_VAL_SHARED 1

/* already (power of 2)-1, i.e. suitable for a mask */
#define LW_SHARED_MASK MAX_BACKENDS
#define LW_LOCK_MASK (MAX_BACKENDS | LW_VAL_EXCLUSIVE)

LWLatch::LWLatch() : _state(0), _waitList() {};

bool LWLatch::attemptLock(LockMode mode) {
    uint32_t oldState;

    oldState = _state.load();

    while (true) {
        uint32_t desiredState;
        bool lockFree;

        desiredState = oldState;
        if (mode == LockMode::Exclusive) {
            lockFree = (oldState & LW_LOCK_MASK) == 0;
            if (lockFree)
                desiredState += LW_VAL_EXCLUSIVE;
        } else {
            lockFree = (oldState & LW_VAL_EXCLUSIVE) == 0;
            if (lockFree)
                desiredState += LW_VAL_SHARED;
        }

        if (_state.compare_exchange_strong(oldState, desiredState)) {
            if (lockFree) {
                return false;
            } else {
                return true;
            }
        }
    }
}

void LWLatch::waitListLock() {
    while (true) {
        uint32_t oldState = _state.fetch_or(LW_FLAG_LOCKED);
        if (!(oldState & LW_FLAG_LOCKED)) {
            // got lock
            return;
        }

        // spin until someone releases lock
        while (!(oldState & LW_FLAG_LOCKED)) {
            Spin::PerformSpin();
            oldState = _state.load();
        }
    }
}

void LWLatch::waitListUnlock() { _state.fetch_and(~LW_FLAG_LOCKED); }

void LWLatch::queueSelf(LockMode mode) {
    auto worker = MyWorker;

    if (worker == nullptr) {
        std::__throw_runtime_error("MyWorker not setup");
    }

    auto &state = worker->GetLockState();
    if (state.status != LockWaitState::NoWait) {
        std::__throw_runtime_error("already got lock");
    }

    this->waitListLock();

    _state.fetch_or(LW_FLAG_HAS_WAITERS);
    auto &lockState = worker->GetLockState();
    lockState.status = LockWaitState::Wait;
    lockState.mode = mode;

    this->_waitList.Push(worker->GetId());

    this->waitListUnlock();
}

void LWLatch::Lock(LockMode mode) {
    int extraWaits = 0;
    auto &sema = MyWorker->GetSemaphore();
    auto &state = MyWorker->GetLockState();

    while (true) {
        bool mustWait = this->attemptLock(mode);

        if (!mustWait) {
            // got lock
            break;
        }

        this->queueSelf(mode);

        mustWait = this->attemptLock(mode);
        if (!mustWait) {
            this->dequeueSelf();
            break;
        }

        while (true) {
            sema.Lock();
            if (state.status == LockWaitState::NoWait)
                break;
            extraWaits++;
        }

        _state.fetch_or(LW_FLAG_RELEASE_OK);
    }

    while (extraWaits-- > 0)
        sema.Unlock();
};

bool LWLatch::TryLock(LockMode mode) { return !this->attemptLock(mode); };

void LWLatch::wakeup() {
    auto wakeup = WorkerListHead{};
    auto wokeup_somebody = false;
    this->waitListLock();

    auto iter = this->_waitList.Iterate();
    while (iter.Next()) {
        auto &waiter = *iter;
        auto id = waiter.GetId();
        auto &state = waiter.GetLockState();

        // all share mode or single X allowed
        if (wokeup_somebody && state.mode == LockMode::Exclusive) {
            continue;
        }

        this->_waitList.Delete(id);
        wakeup.Push(id);

        wokeup_somebody = true;
        
        assert(state.status == LockWaitState::Wait);
        state.status = LockWaitState::Pending;
        
        // 
        if (state.mode == LockMode::Exclusive) {
            break;
        }
    }
    
    assert(wakeup.IsEmpty() || this->_state.load() & LW_FLAG_HAS_WAITERS);
    
    auto oldState = this->_state.load();
    while (true) {
        auto desiredState = oldState;

        // new_release_ok in PG for me always false, because i do not have LW_WAIT_UNTIL_FREE wait mode
        desiredState &= ~LW_FLAG_RELEASE_OK;
        
        if (this->_waitList.IsEmpty()) {
            desiredState &= ~LW_FLAG_HAS_WAITERS;
        }
        
        // release wait lock
        desiredState &= ~LW_FLAG_LOCKED;
        if (this->_state.compare_exchange_strong(oldState, desiredState)) {
            break;
        }
    }
    
    // awaken any waiters removed from queue
    iter = wakeup.Iterate();
    while (iter.Next()) {
        auto &waiter = *iter;
        auto &state = waiter.GetLockState();
        
        wakeup.Delete(waiter.GetId());
        Barrier::Write();
        state.status = LockWaitState::NoWait;
        waiter.GetSemaphore().Unlock();
    }
}

void LWLatch::Release(LockMode mode) {
    uint32_t oldState;

    if (mode == LockMode::Exclusive) {
        oldState = _state.fetch_sub(LW_VAL_EXCLUSIVE);
    } else {
        oldState = _state.fetch_sub(LW_VAL_SHARED);
    }

    constexpr auto wakeFlags = LW_FLAG_HAS_WAITERS | LW_FLAG_RELEASE_OK;
    if ((oldState & wakeFlags) == wakeFlags && (oldState & LW_LOCK_MASK) == 0) {
        this->wakeup();
    }
};
