#pragma once

#include "db/Database.hpp"
#include "transam/ResourceManagerRegistry.hpp"
#include "transam/WriteAheadLog.hpp"
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
    // Write Ahead Log
    extern transam::WriteAheadLog *WALGlobal;
    // Database instance
    extern mi::db::Database *DatabaseGlobal;
    // All registered resource managers
    extern transam::ResourceManagerRegistry *RMgrRegistryGlobal;
}
