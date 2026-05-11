#include "mimidb.hpp"

#include "access/heap/HeapResourceManager.hpp"

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
#include "storage/buffer/BufferManager.hpp"
#include "trans/ResourceManagerId.hpp"
#include "trans/ResourceManagerRegistry.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionManager.hpp"
#include "storage/undo/UndoLog.hpp"
#include "storage/wal/WriteAheadLog.hpp"
#include "worker/Worker.hpp"
#include "worker/WorkerManager.hpp"
#include "worker_state.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>

static constexpr const int MaxWorkers = 16;
static constexpr const int Port = 6543;

static int open_server_socket() {
    auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        throw std::runtime_error("could not initialize socket");
    }

    // SO_REUSEADDR нужен, чтобы когда я постоянно киляю сервер или падает на сегфолте
    // я мог быстро запустить сервер без лизинга ядром.
    const int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        throw std::runtime_error("could not setsockopt(SO_REUSEADDR)");
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
    if (int ret = getaddrinfo(nullptr, std::to_string(Port).c_str(), &hints, &s_i); ret != 0) {
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

static void setupResourceManagers() {
    auto manager = new mi::transam::ResourceManagerRegistry();

    // Heap
    manager->RegisterManager(mi::transam::ResourceManagerId::Heap,
                             mi::access::heap::HeapResourceManager::Create());

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

static void processArguments(int argc, char **argv) {
    for (auto i = 1; i < argc; ++i) {
        auto arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            std::cout << "--help\tshow this help message\n"
                      << "-D [WORKDIR]\tset working directory" << std::endl;
            exit(0);
        }

        if (strcmp(arg, "-D") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "work dir is not provided" << std::endl;
                exit(1);
            }

            i++;
            auto dir = argv[i];
            auto buf = std::array<char, 128>{};
            getcwd(buf.data(), buf.size());
            if (chdir(dir) < 0) {
                std::cerr << "could not chdir to " << dir << ": " << strerror(errno) << std::endl;
                exit(1);
            }

            continue;
        }

        std::cerr << "Unknown option " << arg << std::endl;
        exit(1);
    }
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

int main(int argc, char **argv) {
    processArguments(argc, argv);

    setupStorage();

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
