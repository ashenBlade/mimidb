#pragma once

#include <semaphore>

namespace mi::lock {

class Semaphore {
  private:
    // Core semaphore structure
    std::counting_semaphore<1> _sema;
  public:
    Semaphore();
    
    // Type is not movable nor copyable, because underlying structure is not
    Semaphore(Semaphore &&other) = delete;
    Semaphore &operator=(Semaphore &&other) = delete;

    void Lock();
    void Unlock();
};
}; // namespace mi::lock