#pragma once

#include "worker/WorkerManager.hpp"
#include "storage/BufferManager.hpp"
#include "transam/UndoLog.hpp"
#include "transam/TransactionManager.hpp"

namespace mi {
    // Worker manager for whole cluster
    extern worker::WorkerManager *WorkerGlobal;
    // Buffer pool for whole cluster
    extern storage::BufferManager *BufferPoolGlobal;
    // Undo log
    extern transam::UndoLog *UndoLogGlobal;
    // Transaction manager
    extern transam::TransactionManager *TransactionManagerGlobal;
}
