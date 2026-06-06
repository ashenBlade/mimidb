#include "lock/LWLatch.hpp"
#include "cluster_state.hpp"
#include "lock/Barrier.hpp"
#include "lock/LockList.hpp"
#include "lock/LockMode.hpp"
#include "lock/Spin.hpp"
#include "lock/WorkerLockState.hpp"
#include "worker_state.hpp"
#include <atomic>
#include <barrier>

using namespace mi::lock;

enum LWLatchFlag : uint32_t {
    // Waiters list is locked, so we can perform changes to it
    ListLocked = 1 << 29,
    // When someone releases lock it should check for workers to wakeup
    UnlockWaiter = 1 << 30,
    // Waiters list is not empty
    HasWaiters = static_cast<uint32_t>(1 << 31),
};

// Lower 29 bits stores amount of lockers. We hack this field and for X lock just set
// max value possible, so no S lock can be granted.

static constexpr uint32_t ExclusiveLockValue = (1 << 28);
static constexpr uint32_t SharedLockValue = 1;
static constexpr uint32_t LockCountMask = ExclusiveLockValue | (ExclusiveLockValue - 1);

static_assert((LockCountMask & LWLatchFlag::ListLocked) == 0,
              "Lock mask field must not interfere with latch flags");

LWLatch::LWLatch() : _state(LWLatchFlag::UnlockWaiter), _waiters() {};

// Try to acquire lock in non-blocking way (CAS). Returns true if succeed and
// lock is taken for this mode
bool LWLatch::attemptLock(LockMode mode) {
    auto old = this->_state.load();
    while (true) {
        auto desired = old;
        bool lockFree;

        if (mode == LockMode::Exclusive) {
            lockFree = (old & LockCountMask) == 0;
            if (lockFree) {
                desired += ExclusiveLockValue;
            }
        } else {
            lockFree = (old & LockCountMask) == 0;
            if (lockFree) {
                desired += SharedLockValue;
            }
        }

        if (this->_state.compare_exchange_strong(old, desired)) {
            return lockFree;
        }
    }
}

static void wait_list_lock(std::atomic_uint32_t &state) {
    while (true) {
        auto old = state.fetch_or(LWLatchFlag::ListLocked);
        if (!(old & LWLatchFlag::ListLocked)) {
            // got lock
            break;
        }

        // Spin until got lock
        while (old & LWLatchFlag::ListLocked) {
            Spin::PerformSpin();
            old = state.load();
        }
    }
}

static void wait_list_unlock(std::atomic_uint32_t &state) {
    auto old = state.fetch_and(~LWLatchFlag::ListLocked);
    assert(old & LWLatchFlag::ListLocked);
}

void LWLatch::queueSelf(LockMode mode) {
    auto &worker = LockGlobal->GetWorkerLock(MyWorker->GetId());

    wait_list_lock(this->_state);

    this->_state.fetch_or(LWLatchFlag::HasWaiters);

    worker.SetLockState(WorkerLockState::Waiting);
    worker.SetLockMode(mode);

    this->_waiters.PushTail(MyWorker->GetId());

    wait_list_unlock(this->_state);
}

void LWLatch::dequeueSelf() {
    wait_list_lock(this->_state);

    auto &worker = LockGlobal->GetWorkerLock(MyWorker->GetId());
    auto isWaiting = worker.GetLockState() == WorkerLockState::Waiting;

    if (isWaiting) {
        this->_waiters.Delete(MyWorker->GetId());
    }

    // If we were the only waiters, then remove HasWaiters flag, so on next Unlock
    // we will not check for workers to wakeup
    if (this->_waiters.IsEmpty() && (this->_state.load() & LWLatchFlag::HasWaiters) != 0) {
        this->_state.fetch_and(~LWLatchFlag::HasWaiters);
    }

    wait_list_unlock(this->_state);

    if (isWaiting) {
        // Successfully dequeued ourselves
        worker.SetLockState(WorkerLockState::NotWaiting);
    } else {
        // Somebody woke up us and dequeued

        // Reset UnlockWaiter
        this->_state.fetch_or(LWLatchFlag::UnlockWaiter);

        auto extraWaits = 0;
        while (true) {
            worker.GetSemaphore().acquire();
            if (worker.GetLockState() == WorkerLockState::NotWaiting) {
                break;
            }

            extraWaits++;
        }

        if (extraWaits > 0) {
            worker.GetSemaphore().release(extraWaits);
        }
    }
}

void LWLatch::Lock(LockMode mode) {
    auto &worker = LockGlobal->GetWorkerLock(MyWorker->GetId());
    auto extraWaits = 0;

    while (true) {
        if (this->attemptLock(mode)) {
            // got the lock
            break;
        }

        // We failed to acquire lock on first try, but we can not just queue ourselves
        // to the end of wait list and wait, because by now the lock  could long have been
        // released. Instead add us to the queue and try to grab the lock again.
        //
        // If we succeed we need to revert the queuing and be happy, otherwise we recheck the lock.
        // Otherwise we know that the other locker will see our queue entries when releasing since
        // they existed before we checked for the lock.

        // add to the queue
        this->queueSelf(mode);

        // Now we guaranteed to be woken up if necessary
        if (this->attemptLock(mode)) {
            this->dequeueSelf();
            break;
        }

        // Again failed to grab the lock. Nothing left but only wait for release.
        // Note that we can be woken up spuriously, so track which amount of time
        // we waited for the lock
        while (true) {
            worker.GetSemaphore().acquire();
            if (worker.GetLockState() == WorkerLockState::NotWaiting) {
                break;
            }

            // spurious wakeup
            extraWaits++;
        }

        // Allow Unlock to release waiters again
        this->_state.fetch_or(LWLatchFlag::UnlockWaiter);
    }

    if (extraWaits > 0) {
        worker.GetSemaphore().release(extraWaits);
    }
};

void LWLatch::wakeup() {
    // List of workers to wakeup
    LockListHead wakeup{};

    wait_list_lock(this->_state);

    auto wokeupSomebody = false;
    for (auto it = this->_waiters.begin(); it != this->_waiters.end(); ++it) {
        auto &waiter = LockGlobal->GetWorkerLock(it.Current);

        if (wokeupSomebody && waiter.GetLockMode() == LockMode::Exclusive) {
            // We can wakeup only 1 X lock, so skip if anyone was seen so far
            continue;
        }

        this->_waiters.Delete(it.Current);
        wakeup.PushTail(it.Current);

        // Do not wakeup other X locks
        wokeupSomebody = true;

        assert(waiter.GetLockState() == WorkerLockState::Waiting);
        waiter.SetLockState(WorkerLockState::PendingWakeup);

        if (waiter.GetLockMode() == LockMode::Exclusive) {
            // Only 1 X lock can be granted
            break;
        }
    }

    assert(wakeup.IsEmpty() || (this->_state.load() & LWLatchFlag::HasWaiters) != 0);

    auto old = this->_state.load();
    while (true) {
        auto desired = old;

        // Prevent additional wakeups until retryer gets to run.
        // If there are some waiters, then they will set this flag after wakeup,
        // otherwise concurrent Unlock will perform additional wakeup of other
        // workers.
        desired &= ~LWLatchFlag::UnlockWaiter;

        if (this->_waiters.IsEmpty()) {
            desired &= ~LWLatchFlag::HasWaiters;
        }

        // Release lock at same moment we set flags (for performance).
        // Do not
        desired &= ~LWLatchFlag::ListLocked;

        if (this->_state.compare_exchange_strong(old, desired)) {
            break;
        }
    }

    for (auto it = wakeup.begin(); it != wakeup.end(); ++it) {
        auto &waiter = LockGlobal->GetWorkerLock(it.Current);

        wakeup.Delete(it.Current);

        // Make sure we remove worker from list BEFORE updating it's lock state.
        // Otherwise worker can be woken up for some other reason and enqueue
        // for a new lock - if this happens list would end up being corrupted.
        Barrier::Write();
        waiter.SetLockState(WorkerLockState::NotWaiting);
        Barrier::Write();
        waiter.GetSemaphore().release();
    }
}

void LWLatch::Unlock(LockMode mode) {
    uint32_t old;

    if (mode == LockMode::Exclusive) {
        old = this->_state.fetch_sub(ExclusiveLockValue);

        // Verify that we were holding X lock
        assert(old & ExclusiveLockValue);
    } else {
        old = this->_state.fetch_sub(SharedLockValue);
    }
    assert((this->_state.load() & ExclusiveLockValue) == 0);
    constexpr auto checkWaitersFlags = (LWLatchFlag::HasWaiters | LWLatchFlag::UnlockWaiter);
    // On latch release we should wake any waiters not only when waiters count drops,
    // but also take into account if we allowed to wakeup any (special flags)
    if ((old & checkWaitersFlags) == checkWaitersFlags &&
        // Returned value is OLD, so we must check either we were X locker or the only S
        (mode == LockMode::Exclusive || (old & LockCountMask) == 1)) {
        this->wakeup();
    }
};

LWLatch::~LWLatch() {
    // No one must hold any lock when we destroying latch
    assert((this->_state.load() & LockCountMask) == 0);
}
