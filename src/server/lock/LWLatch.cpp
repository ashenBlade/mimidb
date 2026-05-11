#include "lock/LWLatch.hpp"

using namespace mi::lock;

LWLatch::LWLatch() : _mutex() {};

void LWLatch::Lock(LockMode mode) {
    if (mode == LockMode::Exclusive) {
        this->_mutex.lock();
    } else {
        this->_mutex.lock_shared();
    }
};

bool LWLatch::TryLock(LockMode mode) {
    if (mode == LockMode::Exclusive) {
        return this->_mutex.try_lock();
    } else {
        return this->_mutex.try_lock_shared();
    }
};

void LWLatch::Release(LockMode mode) {
    if (mode == LockMode::Exclusive) {
        this->_mutex.unlock();
    } else {
        this->_mutex.unlock_shared();
    }
};
