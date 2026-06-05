#include "SpinLock.hpp"
#include <atomic>

using namespace mi::lock;

// Implementation is taken from https://en.cppreference.com/cpp/atomic/atomic_flag

// Initially unlocked
SpinLock::SpinLock() : _value(false) {};

void SpinLock::Lock() {
    while (this->_value.test_and_set(std::memory_order_acquire)) {
        this->_value.wait(true, std::memory_order_relaxed);
    }
}

bool SpinLock::TryLock() {
    return !this->_value.test_and_set(std::memory_order_acquire);
}

void SpinLock::Unlock() {
    this->_value.clear(std::memory_order::release);
    this->_value.notify_one();
}
