#pragma once

#include <atomic>

namespace mi::lock {

class Barrier {
  public:
    /// @brief Write barrier
    static inline void Write() { std::atomic_thread_fence(std::memory_order_release); };
};

} // namespace mi::lock