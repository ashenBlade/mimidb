#include "cluster_state.hpp"
#include "access/heap/HeapResourceManager.hpp"
#include "access/heap/HeapTable.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/ITable.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "db/bulitin/int.hpp"
#include "db/catalog/ColumnInfo.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeId.hpp"
#include "include/db/bulitin/int.hpp"
#include "include/db/catalog/OperatorId.hpp"
#include "include/db/catalog/OperatorInfo.hpp"
#include "include/db/catalog/TableInfo.hpp"
#include "include/db/catalog/TypeId.hpp"
#include "include/executor/Oid.hpp"
#include "logger.hpp"
#include "logger/Logger.hpp"
#include "mi_config.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

// Global variables
mi::db::Database *mi::DatabaseGlobal;
mi::worker::WorkerManager *mi::WorkerGlobal;
mi::storage::buffer::BufferManager *mi::BufferPoolGlobal;
mi::storage::undo::UndoLog *mi::UndoLogGlobal;
mi::storage::trans::TransactionManager *mi::TransactionManagerGlobal;
mi::storage::wal::WriteAheadLog *mi::WALGlobal;
mi::storage::trans::ResourceManagerRegistry *mi::RMgrRegistryGlobal;
mi::logger::Logger *mi::LoggerGlobal;

static void setupDatabase() {
    // CREATE TABLE tbl(a int4, b int2);

    // Tuple with 2 attributes, both ints
    auto tupDesc = std::make_unique<mi::access::table::TupleDescriptor>(std::vector{
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int32, sizeof(int32_t),
                                               true},
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int16, sizeof(int16_t),
                                               true},
    });
    auto table = std::make_unique<mi::access::heap::HeapTable>(
        mi::schema::catalog::TableId::MainTableId, std::move(tupDesc));

    auto columns =
        std::vector<mi::db::catalog::ColumnInfo>{mi::db::catalog::ColumnInfo{
                                                     mi::schema::catalog::TypeId::Int32,
                                                     "a",
                                                     mi::access::table::AttrNumber::Min(),
                                                 },
                                                 mi::db::catalog::ColumnInfo{
                                                     mi::schema::catalog::TypeId::Int16,
                                                     "b",
                                                     mi::access::table::AttrNumber::Min() + 1U,
                                                 }};

    // Table info
    auto tables = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::TableInfo>>{};
    tables.emplace(
        mi::schema::catalog::TableId::MainTableId,
        std::make_unique<mi::db::catalog::TableInfo>(mi::schema::catalog::TableId::MainTableId,
                                                     table->GetDescriptor(), std::move(columns)));

    // Type info
    auto types = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::TypeInfo>>{};
    types.emplace(mi::schema::catalog::TypeId::Int16,
                  std::make_unique<mi::db::catalog::TypeInfo>(mi::schema::catalog::TypeId::Int16,
                                                              sizeof(int16_t),
                                                              mi::db::builtin::Int16Output));
    types.emplace(mi::schema::catalog::TypeId::Int32,
                  std::make_unique<mi::db::catalog::TypeInfo>(mi::schema::catalog::TypeId::Int32,
                                                              sizeof(int32_t),
                                                              mi::db::builtin::Int32Output));
    types.emplace(mi::schema::catalog::TypeId::Int64,
                  std::make_unique<mi::db::catalog::TypeInfo>(mi::schema::catalog::TypeId::Int64,
                                                              sizeof(int64_t),
                                                              mi::db::builtin::Int64Output));

    // Operator info
    auto operators = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::OperatorInfo>>{};
    operators.emplace(mi::db::catalog::OperatorId::EqInt32Int32,
                      std::make_unique<mi::db::catalog::OperatorInfo>(
                          mi::db::catalog::OperatorId::EqInt32Int32,
                          mi::schema::catalog::TypeId::Int32, mi::schema::catalog::TypeId::Int32,
                          mi::db::catalog::OperatorStrategy::Equal, mi::db::builtin::Int32Eq));

    auto schema =
        std::make_unique<mi::db::Schema>(std::move(tables), std::move(types), std::move(operators));

    auto itables = std::unordered_map<mi::Oid, std::unique_ptr<mi::access::table::ITable>>{};
    itables.emplace(mi::schema::catalog::TableId::MainTableId, std::move(table));

    mi::DatabaseGlobal = new mi::db::Database(std::move(schema), std::move(itables));
}

static void setupStorage() {
    // Пока у меня не полноценная поддержка диска и только в памяти все делаю.
    // Не хочу возиться с поддержкой всякого говна по типу восстановления, поэтому
    // каждый раз на старте буду создавать новые/удалять старые файлы, чтобы с нуля начинать

    // Данные
    struct stat s;
    errno = 0;

    if (stat("data/1", &s) < 0) {
        if (errno == EEXIST) {
            auto fd = creat("data/1", 0666);
            if (fd < 0) {
                std::cerr << "could not creat data/1" << std::endl;
                exit(1);
            }
            close(fd);
        } else {
            std::cerr << "could not stat: " << strerror(errno) << std::endl;
            exit(1);
        }
    } else if (s.st_size > 0) {
        if (truncate64("data/1", 0) < 0) {
            std::cerr << "could not truncate: " << strerror(errno) << std::endl;
            exit(1);
        }
    }

    // WAL/UNDO
    auto fd = creat("undo", 0666);
    if (fd < 0) {
        std::cerr << "could not open undo: " << strerror(errno) << std::endl;
        exit(1);
    }
    close(fd);

    fd = creat("wal", 0666);
    if (fd < 0) {
        std::cerr << "could not open wal: " << strerror(errno) << std::endl;
        exit(1);
    }
    close(fd);
}

static void setupResourceManagers() {
    auto manager = new mi::storage::trans::ResourceManagerRegistry();

    // Heap
    manager->RegisterManager(mi::storage::trans::ResourceManagerId::Heap,
                             mi::access::heap::HeapResourceManager::Create());
    mi::RMgrRegistryGlobal = manager;
}

void setupCluster() {
    setupResourceManagers();
    setupDatabase();

    // temp
    setupStorage();

    // Create global structures
    mi::WorkerGlobal = new mi::worker::WorkerManager(mi::Config::MaxWorkers);
    mi::BufferPoolGlobal = new mi::storage::buffer::BufferManager();
    mi::TransactionManagerGlobal = new mi::storage::trans::TransactionManager();
    mi::UndoLogGlobal = mi::storage::undo::UndoLog::Open("undo");
    mi::WALGlobal = mi::storage::wal::WriteAheadLog::Open("wal");
    mi::LoggerGlobal = new mi::logger::Logger();
}
