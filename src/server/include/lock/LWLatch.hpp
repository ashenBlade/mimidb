#pragma once

#include "lock/LockMode.hpp"

#include <shared_mutex>

namespace mi::lock {

class LWLatch {
  private:
    /// @brief Actual lock
    std::shared_mutex _mutex;

  public:
    LWLatch();

    void Lock(LockMode mode);
    bool TryLock(LockMode mode);
    void Release(LockMode mode);
};

}; // namespace mi::lock
