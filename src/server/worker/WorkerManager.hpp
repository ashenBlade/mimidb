#pragma once

#include "worker/Worker.hpp"
#include <assert.h>
#include <vector>

namespace mi::worker {

class WorkerManager {
  private:
    /// @brief Running workers
    std::vector<Worker> _workers;

  public:
    WorkerManager(int workersCount);

    Worker *GetWorker(WorkerId id) {
        assert(id.IsValid());
        assert(static_cast<size_t>(id.value) < this->_workers.size());
        return &this->_workers[static_cast<size_t>(id.value)];
    }
    size_t WorkersCount() const { return this->_workers.size(); }

    /// @brief Start new session for newly accepted connection
    bool StartNewSession(int sock);
};

} // namespace mi::worker
