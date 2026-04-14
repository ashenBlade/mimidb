#pragma once

#include "lock/LockMode.hpp"

#include <shared_mutex>

namespace mi::lock {
class Latch {
  private:
    

  public:
    Latch();

    Latch &operator=(const Latch &other) = delete;
    Latch(const Latch &other) = delete;

    Latch &operator=(Latch &&other) = delete;
    Latch(Latch &&other) = delete;

    void Lock();
    void Release();
};
}; // namespace mi::lock
