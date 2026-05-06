#include "access/heap/HeapResourceManager.hpp"
#include "mimidb.hpp"

#include "access/table/Oid.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "cluster_state.hpp"
#include "db/Database.hpp"
#include "db/Schema.hpp"
#include "db/bulitin/int.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TableInfo.hpp"
#include "db/catalog/TypeId.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "storage/BufferManager.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/ResourceManagerRegistry.hpp"
#include "transam/Transaction.hpp"
#include "transam/TransactionManager.hpp"
#include "transam/UndoLog.hpp"
#include "transam/WriteAheadLog.hpp"
#include "worker/Worker.hpp"
#include "worker/WorkerManager.hpp"
#include "worker_state.hpp"

#include <csignal>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unordered_map>

static constexpr const int MaxWorkers = 16;

static int open_server_socket() {
    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        throw std::runtime_error("could not initialize socket");
    }

    addrinfo hints = {
        // AI_PASSIVE - для bind, AI_NUMERICSERV - не разрешать имена
        .ai_flags = AI_PASSIVE | AI_NUMERICSERV,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,

        // Занулены
        .ai_addrlen = 0,
        .ai_addr = 0,
        .ai_canonname = 0,
        .ai_next = nullptr,
    };

    addrinfo *s_i = nullptr;
    const int port = 6543;
    if (int ret = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &s_i); ret != 0) {
        throw std::logic_error(gai_strerror(ret));
    }

    if (bind(sock, s_i->ai_addr, s_i->ai_addrlen) < 0) {
        throw std::logic_error("could not bind address");
    }
    
    freeaddrinfo(s_i);
    
    if (listen(sock, 128) < 0) {
        throw std::logic_error("could not listen");
    }

    return sock;
}

static volatile bool stop_requested = false;

static void sigint_handler(int) {
    stop_requested = true;
    kill(getpid(), SIGURG);
}

static void setupDatabase() {
    // CREATE TABLE tbl(a int4, b int2);

    // Tuple with 2 attributes, both ints
    auto desc = mi::access::table::TupleDescriptor{std::vector {
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int32, sizeof(int32_t), true},
        mi::access::table::AttributeDescriptor{mi::schema::catalog::TypeId::Int16, sizeof(int32_t), true},
        
    }};
    auto tables = std::unordered_map<mi::Oid, mi::db::catalog::TableInfo>{
        {mi::schema::catalog::TableId::MainTableId, mi::db::catalog::TableInfo{mi::schema::catalog::TableId::MainTableId, std::move(desc)}}
    };

    // Only scalar ints are supported now
    auto types = std::unordered_map<mi::Oid, mi::db::catalog::TypeInfo> {
        {mi::schema::catalog::TypeId::Int16, mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int16, sizeof(int16_t), mi::db::builtin::Int16Output}},
        {mi::schema::catalog::TypeId::Int32, mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int32, sizeof(int32_t), mi::db::builtin::Int32Output}},
        {mi::schema::catalog::TypeId::Int64, mi::db::catalog::TypeInfo{mi::schema::catalog::TypeId::Int64, sizeof(int64_t), mi::db::builtin::Int64Output}},
    };
    auto schema = std::make_unique<mi::db::Schema>(std::move(tables), std::move(types));
    mi::DatabaseGlobal = new mi::db::Database(std::move(schema));
}

static void setupResourceManagers() {
    auto manager = new mi::transam::ResourceManagerRegistry();

    // Heap
    manager->RegisterManager(mi::transam::ResourceManagerId::Heap, mi::access::heap::HeapResourceManager::Create());

    mi::RMgrRegistryGlobal = manager;
}

// Global variables declarations
// worker_state
thread_local mi::worker::Worker *mi::MyWorker;
thread_local mi::transam::Transaction *mi::MyTransaction;

// cluster_state
mi::db::Database *mi::DatabaseGlobal;
mi::worker::WorkerManager *mi::WorkerGlobal;
mi::storage::BufferManager *mi::BufferPoolGlobal;
mi::transam::UndoLog *mi::UndoLogGlobal;
mi::transam::TransactionManager *mi::TransactionManagerGlobal;
mi::transam::WriteAheadLog *mi::WALGlobal;
mi::transam::ResourceManagerRegistry *mi::RMgrRegistryGlobal;

static int main_loop() {
    // Create new server socket
    auto server = open_server_socket();
    while (true) {
        auto clientSock = accept(server, nullptr, nullptr);
        if (clientSock < 0) {
            if (errno == EINTR && stop_requested) {
                std::cerr << "got SIGINT, stop working" << std::endl;
            } else {
                std::cerr << "error accepting client, stop working" << std::endl;
            }
            break;
        }

        mi::WorkerGlobal->StartNewSession(clientSock);
    }

    shutdown(server, SHUT_RDWR);
    close(server);
    return 0;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    // Create global structures
    mi::WorkerGlobal = new mi::worker::WorkerManager(MaxWorkers);
    mi::BufferPoolGlobal = new mi::storage::BufferManager();
    mi::TransactionManagerGlobal = new mi::transam::TransactionManager();
    mi::UndoLogGlobal = mi::transam::UndoLog::Open("undo");
    mi::WALGlobal = mi::transam::WriteAheadLog::Open("wal");

    setupResourceManagers();

    // There is only 1 database, so no need to setup this in worker
    setupDatabase();

    // Register simple handler to stop processing
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::cerr << "could not setup sigint handler" << std::endl;
        return 1;
    }

    // Create new server socket
    auto ret = main_loop();
    if (ret != 0) {
        std::cerr << "invalid return code " << ret << std::endl;
    }

    return ret;
}
