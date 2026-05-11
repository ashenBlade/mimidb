#include "cluster_state.hpp"
#include "access/heap/HeapResourceManager.hpp"
#include "db/bulitin/int.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeId.hpp"
#include "mi_config.hpp"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

// Global variables
mi::db::Database *mi::DatabaseGlobal;
mi::worker::WorkerManager *mi::WorkerGlobal;
mi::storage::buffer::BufferManager *mi::BufferPoolGlobal;
mi::storage::undo::UndoLog *mi::UndoLogGlobal;
mi::storage::trans::TransactionManager *mi::TransactionManagerGlobal;
mi::storage::wal::WriteAheadLog *mi::WALGlobal;
mi::storage::trans::ResourceManagerRegistry *mi::RMgrRegistryGlobal;

static void setupDatabase() {
    // CREATE TABLE tbl(a int4, b int2);

    // Tuple with 2 attributes, both ints
    auto desc = mi::access::table::TupleDescriptor{std::vector{
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int32, sizeof(int32_t),
                                               true},
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int16, sizeof(int32_t),
                                               true},

    }};
    auto tables = std::unordered_map<mi::Oid, mi::db::catalog::TableInfo>{
        {mi::schema::catalog::TableId::MainTableId,
         mi::db::catalog::TableInfo{mi::schema::catalog::TableId::MainTableId, std::move(desc)}}};

    // Only scalar ints are supported now
    auto types = std::unordered_map<mi::Oid, mi::db::catalog::TypeInfo>{
        {mi::schema::catalog::TypeId::Int16,
         mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int16, sizeof(int16_t),
                                   mi::db::builtin::Int16Output}},
        {mi::schema::catalog::TypeId::Int32,
         mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int32, sizeof(int32_t),
                                   mi::db::builtin::Int32Output}},
        {mi::schema::catalog::TypeId::Int64,
         mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int64, sizeof(int64_t),
                                   mi::db::builtin::Int64Output}},
    };

    auto schema = std::make_unique<mi::db::Schema>(std::move(tables), std::move(types));
    mi::DatabaseGlobal = new mi::db::Database(std::move(schema));
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
}