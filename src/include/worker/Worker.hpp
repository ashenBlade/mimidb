#pragma once

#include <unistd.h>

#include <memory>

#include "lock/LockState.hpp"
#include "lock/Semaphore.hpp"
#include "worker/WorkerId.hpp"

namespace mi::worker {
class Worker {
  private:
    /// @brief Assigned id for worker, index in global array
    WorkerId _id;

    /// @brief State for lock support
    mi::lock::LockState _lockState;

    /// @brief Semaphore to lock on
    std::unique_ptr<mi::lock::Semaphore> _sema;

    void swap(Worker &other) noexcept;

  public:
    /// @brief Creates new Worker object with InvalidId
    Worker();
    /// @brief Create new Worker and immediately assign it's id
    Worker(WorkerId id);
    
    // Mark noexcept to use std::swap
    Worker(Worker &&other) noexcept;
    Worker &operator=(Worker &&other) noexcept;

    // Copying is not allowed
    Worker(const Worker &worker) = delete;
    Worker &operator=(const Worker& other) = delete;

    WorkerId GetId() const;

    mi::lock::LockState &GetLockState();
    mi::lock::Semaphore &GetSemaphore();
};

} // namespace mi::worker