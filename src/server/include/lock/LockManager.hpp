#pragma once

#include "lock/WorkerLock.hpp"
#include "worker/WorkerId.hpp"
#include <cassert>
#include <cstddef>
#include <vector>
namespace mi::lock {

class LockManager {
  private:
    std::vector<WorkerLock> _states;

  public:
    LockManager(std::size_t workersCount);
    WorkerLock &GetWorkerLock(worker::WorkerId id) {
        assert(id.IsValid());
        return this->_states[static_cast<std::size_t>(id.value)];
    }
};

} // namespace mi::lock
