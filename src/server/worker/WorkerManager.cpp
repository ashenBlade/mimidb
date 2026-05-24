#include "worker/WorkerManager.hpp"
#include <algorithm>

using namespace mi::worker;

WorkerManager::WorkerManager(int workersCount) : _workers() {
    for (int i = 0; i < workersCount; i++) {
        _workers.emplace_back(WorkerId{i});
    }

    // Worker with Id 0 is always reserved for master
    _workers[0].SetBusy();
};

bool WorkerManager::StartNewSession(int sock) {
    auto worker = std::find_if(this->_workers.begin(), this->_workers.end(),
                               [](const Worker &worker) { return !worker.IsBusy(); });
    if (worker == this->_workers.end()) {
        return false;
    }

    worker->Submit(sock);
    return true;
}
