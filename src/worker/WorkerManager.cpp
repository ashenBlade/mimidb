#include "mimidb.hpp"

#include <exception>
#include <mutex>
#include <algorithm>
#include <iostream>

#include "worker/WorkerManager.hpp"
#include "worker/Handler.hpp"

using namespace mi::worker;

WorkerManager::WorkerManager(int workersCount) : _workers() {
    for (int i = 0; i < workersCount; i++) {
        _workers.emplace_back(Worker{i});
    }    
};

bool WorkerManager::StartNewSession(int sock) {
    auto g = std::lock_guard{this->_lock};
    for (auto it = this->_workers.begin(); it != this->_workers.end(); ++it) {
        if (it->IsBusy()) {
            continue;
        }
    }

    auto worker = std::find_if(this->_workers.begin(), this->_workers.end(), [](const Worker &worker) { return !worker.IsBusy();});
    if (worker == this->_workers.end()) {
        return false;
    }

    worker->Submit(sock);
    return true;
}