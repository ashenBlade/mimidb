#include <iostream>
#include <thread>

#include "mimidb.hpp"

#include "worker_state.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    // For now leave only 16 workers, i dont need more
    constexpr const int maxWorkers = 16;

    // Setup global worker manager
    auto workerGlob = mi::worker::WorkerGlobal = new mi::worker::WorkerManager{maxWorkers};

    // TODO: создание непосредственных воркеров-потоков

    return 0;
}
