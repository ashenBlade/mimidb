#include "mimidb.hpp"

#include "worker/WorkerManager.hpp"

using namespace mi::worker;

WorkerManager::WorkerManager(int workersCount) : _workers() {
    for (size_t i = 0; i < workersCount; i++) {
        _workers.emplace_back(Worker{i});
    }
    
};

