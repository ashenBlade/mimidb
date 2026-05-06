#include "mimidb.hpp"

#include "lock/Semaphore.hpp"

using namespace mi::lock;

Semaphore::Semaphore(): _sema(1) {};

void Semaphore::Lock() {
    this->_sema.acquire();
}

void Semaphore::Unlock() {
    this->_sema.release();
}
