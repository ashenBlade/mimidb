#include "cluster_state.hpp"
#include "Settings.hpp"
#include "access/AttrNumber.hpp"
#include "access/ITable.hpp"
#include "access/TupleDescriptor.hpp"
#include "access/heap/HeapResourceManager.hpp"
#include "access/heap/HeapTable.hpp"
#include "cluster_state.hpp"
#include "db/builtin/int.hpp"
#include "db/builtin/text.hpp"
#include "db/catalog/ColumnInfo.hpp"
#include "db/catalog/OperatorId.hpp"
#include "db/catalog/OperatorInfo.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TableInfo.hpp"
#include "db/catalog/TypeId.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "executor/Oid.hpp"
#include "lock/LockManager.hpp"
#include "logger.hpp"
#include "logger/ConsoleLogHandler.hpp"
#include "logger/DefaultLogFormatter.hpp"
#include "logger/FileLogHandler.hpp"
#include "logger/ILogHandler.hpp"
#include "logger/Logger.hpp"
#include "mi_config.hpp"
#include "storage/io/File.hpp"
#include "worker_state.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/limits.h>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <unordered_map>

// Global variables
mi::db::Database *mi::DatabaseGlobal;
mi::worker::WorkerManager *mi::WorkerGlobal;
mi::lock::LockManager *mi::LockGlobal;
mi::storage::buffer::BufferManager *mi::BufferPoolGlobal;
mi::storage::undo::UndoLog *mi::UndoLogGlobal;
mi::storage::trans::TransactionManager *mi::TransactionManagerGlobal;
mi::storage::wal::WriteAheadLog *mi::WALGlobal;
mi::storage::trans::ResourceManagerRegistry *mi::RMgrRegistryGlobal;
mi::logger::Logger *mi::LoggerGlobal;

static void setupDatabase() {
    // CREATE TABLE tbl(a int4, b int2);

    // Tuple with 2 attributes, both ints
    auto tupDesc = std::make_unique<mi::access::TupleDescriptor>(std::vector{
        mi::access::AttributeDescriptor{mi::schema::catalog::TypeId::Int32, sizeof(int32_t), true},
        mi::access::AttributeDescriptor{mi::schema::catalog::TypeId::Int16, sizeof(int16_t), true},
    });
    auto table = std::make_unique<mi::access::heap::HeapTable>(
        mi::schema::catalog::TableId::MainTableId, std::move(tupDesc));

    auto columns = std::vector<mi::db::catalog::ColumnInfo>{mi::db::catalog::ColumnInfo{
                                                                mi::schema::catalog::TypeId::Int32,
                                                                "a",
                                                                mi::access::AttrNumber::Min(),
                                                            },
                                                            mi::db::catalog::ColumnInfo{
                                                                mi::schema::catalog::TypeId::Int16,
                                                                "b",
                                                                mi::access::AttrNumber::Min() + 1U,
                                                            }};

    // Table info
    auto tables = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::TableInfo>>{};
    tables.emplace(
        mi::schema::catalog::TableId::MainTableId,
        std::make_unique<mi::db::catalog::TableInfo>(mi::schema::catalog::TableId::MainTableId,
                                                     table->GetDescriptor(), std::move(columns)));

    // Types
    auto types = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::TypeInfo>>{};
    for (const auto &[typid, size, output] : {
             std::make_tuple(mi::schema::catalog::TypeId::Int16,
                             static_cast<ssize_t>(sizeof(int16_t)), mi::db::builtin::Int16Output),
             std::make_tuple(mi::schema::catalog::TypeId::Int32,
                             static_cast<ssize_t>(sizeof(int32_t)), mi::db::builtin::Int32Output),
             std::make_tuple(mi::schema::catalog::TypeId::Int64,
                             static_cast<ssize_t>(sizeof(int64_t)), mi::db::builtin::Int64Output),
             std::make_tuple(mi::schema::catalog::TypeId::Text, mi::db::catalog::TypeSizeVariable,
                             mi::db::builtin::TextOutput),
         }) {
        types.emplace(typid, std::make_unique<mi::db::catalog::TypeInfo>(typid, size, output));
    }

    // Equality operators
    auto operators = std::unordered_map<mi::Oid, std::unique_ptr<mi::db::catalog::OperatorInfo>>{};
    for (const auto &[opid, typid, func] : {
             std::make_tuple(mi::db::catalog::OperatorId::EqInt16Int16,
                             mi::schema::catalog::TypeId::Int16, mi::db::builtin::Int16Eq),
             std::make_tuple(mi::db::catalog::OperatorId::EqInt32Int32,
                             mi::schema::catalog::TypeId::Int32, mi::db::builtin::Int32Eq),
             std::make_tuple(mi::db::catalog::OperatorId::EqInt64Int64,
                             mi::schema::catalog::TypeId::Int64, mi::db::builtin::Int64Eq),
             std::make_tuple(mi::db::catalog::OperatorId::EqTextText,
                             mi::schema::catalog::TypeId::Text, mi::db::builtin::TextEq),
         }) {
        operators.emplace(opid,
                          std::make_unique<mi::db::catalog::OperatorInfo>(
                              opid, typid, typid, mi::db::catalog::OperatorStrategy::Equal, func));
    }

    auto schema =
        std::make_unique<mi::db::Schema>(std::move(tables), std::move(types), std::move(operators));

    auto itables = std::unordered_map<mi::Oid, std::unique_ptr<mi::access::ITable>>{};
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

static void setupMasterWorker() {
    auto masterWorkerId = mi::worker::WorkerId{0};
    auto worker = mi::WorkerGlobal->GetWorker(masterWorkerId);
    mi::MyWorker = worker;
}

static void setupLogger(mi::Settings &settings) {
    auto formatter = std::make_unique<mi::logger::DefaultLogFormatter>();
    std::unique_ptr<mi::logger::ILogHandler> handler;
    if (settings.LogFile.size()) {
        auto fd = open(settings.LogFile.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
        auto file = mi::storage::io::File{fd};
        handler = std::make_unique<mi::logger::FileLogHandler>(std::move(file));
    } else {
        handler = std::make_unique<mi::logger::ConsoleLogHandler>();
    }

    mi::LoggerGlobal = new mi::logger::Logger(std::move(handler), std::move(formatter));
}

static void setupDirectory(mi::Settings &settings) {
    if (settings.DataDirectory.empty()) {
        throw std::logic_error("Data directory is not specified");
    }

    if (chdir(settings.DataDirectory.c_str()) < 0) {
        throw std::runtime_error(std::string{"could not change directory: "} + strerror(errno));
    }
}

void setupCluster(mi::Settings &settings) {
    setupLogger(settings);
    setupDirectory(settings);

    setupResourceManagers();
    setupDatabase();

    // Временное решение
    setupStorage();

    // Create global structures
    mi::WorkerGlobal = new mi::worker::WorkerManager(mi::Config::MaxWorkers);
    mi::LockGlobal = new mi::lock::LockManager(mi::Config::MaxWorkers);
    mi::TransactionManagerGlobal =
        new mi::storage::trans::TransactionManager(mi::Config::MaxWorkers);
    mi::BufferPoolGlobal = new mi::storage::buffer::BufferManager(mi::Config::BufferPoolSize);
    mi::UndoLogGlobal = mi::storage::undo::UndoLog::Open("undo");
    mi::WALGlobal = mi::storage::wal::WriteAheadLog::Open("wal");

    setupMasterWorker();
}
