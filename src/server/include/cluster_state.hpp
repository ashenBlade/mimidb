#pragma once

#include "db/Database.hpp"
#include "storage/buffer/BufferManager.hpp"
#include "storage/undo/UndoLog.hpp"
#include "storage/wal/WriteAheadLog.hpp"
#include "trans/ResourceManagerRegistry.hpp"
#include "trans/TransactionManager.hpp"
#include "worker/WorkerManager.hpp"

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
} // namespace mi
