#pragma once

#include "worker/WorkerManager.hpp"
#include "storage/BufferManager.hpp"

namespace mi {
    // Worker manager for whole cluster
    extern mi::worker::WorkerManager *WorkerGlobal;
    // Buffer pool for whole cluster
    extern mi::storage::BufferManager *BufferPoolGlobal;
}
