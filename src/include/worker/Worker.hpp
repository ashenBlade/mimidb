#pragma once

#include <unistd.h>

#include <memory>
#include <thread>

#include "worker/WorkerId.hpp"

namespace mi::worker {
class Worker {
  private:
    /// @brief Assigned id for worker, index in global array
    WorkerId _id;

    /// @brief Thread for this worker
    std::thread _thread;

    /// @brief Worker is busy processing the connection
    bool _busy;

    void swap(Worker &other) noexcept;
    
    static void HandleUserConnectionGuts(WorkerId id, int sock);

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

    /// @brief Worker is processing session right now
    bool IsBusy() const;

    /// @brief Start new worker with for this client
    void Submit(int sock);
};

} // namespace mi::worker