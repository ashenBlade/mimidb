#pragma once

#include <atomic>
namespace mi::lock {
class SpinLock {
  private:
    std::atomic_flag _value;

  public:
    SpinLock();

    void Lock();
    bool TryLock();
    void Unlock();

    void lock() { this->Lock(); }

    bool try_lock() { return this->TryLock(); }

    void unlock() { this->Unlock(); }
};
} // namespace mi::lock
