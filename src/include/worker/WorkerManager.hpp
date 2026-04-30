#pragma once

#include <assert.h>
#include <vector>
#include <shared_mutex>

#include "worker/Worker.hpp"

namespace mi::worker {

class WorkerManager {
  private:
  /// @brief Running workers
  std::vector<Worker> _workers;

  /// @brief Lock for manipulating contents
  std::shared_mutex _lock;

  public:
    WorkerManager(int workersCount);

    Worker *GetWorker(WorkerId id) {
      assert(id.IsValid());
      return &this->_workers[static_cast<size_t>(id.value)];
    }
    size_t WorkersCount() const {
      return this->_workers.size();
    }

    /// @brief Start new session for newly accepted connection
    bool StartNewSession(int sock);
};

} // namespace mi::worker
