#pragma once

#include <vector>

#include "worker/Worker.hpp"

namespace mi::worker {

class WorkerManager {
  private:
    /// @brief Running workers
    std::vector<Worker> _workers;

  public:
    WorkerManager(int workersCount);

    Worker &GetWorker(int id);
    int WorkersCount() const;
};

} // namespace mi::worker
