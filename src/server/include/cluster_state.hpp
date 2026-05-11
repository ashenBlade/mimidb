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
extern storage::buffer::BufferManager *BufferPoolGlobal;
// Undo log
extern storage::undo::UndoLog *UndoLogGlobal;
// Transaction manager
extern storage::trans::TransactionManager *TransactionManagerGlobal;
// Write Ahead Log
extern storage::wal::WriteAheadLog *WALGlobal;
// Database instance
extern mi::db::Database *DatabaseGlobal;
// All registered resource managers
extern storage::trans::ResourceManagerRegistry *RMgrRegistryGlobal;
} // namespace mi
