#include "worker/Handler.hpp"
#include "MimiClient.hpp"
#include "access/table/AttrNumber.hpp"
#include "executor/Datum.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "cluster_state.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "executor/VirtualTuple.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionManager.hpp"
#include "worker/WorkerManager.hpp"
#include "worker_state.hpp"
#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace mi::worker;
using AttrNumber = mi::access::table::AttrNumber;
using MimiClient = mi::interface::libmimi::MimiClient;

enum CommandType {
    /* Start from 1 to use as condition in 'while' loop */
    // CRUD
    SELECT = 1,
    INSERT = 2,
    UPDATE = 3,
    DELETE = 4,

    // TCL
    BEGIN = 5,
    COMMIT = 6,
    ROLLBACK = 7,
};

class SocketServer {
  private:
    WorkerId _id;
    MimiClient _client;

    // Vector of output functions for each attribute
    std::optional<std::vector<mi::db::catalog::TypeInfo::OutputFunction>> _outputs;

    const std::vector<mi::db::catalog::TypeInfo::OutputFunction> &
    getOutputFunctions(const mi::access::table::TupleDescriptor &desc) {
        if (!this->_outputs.has_value()) {
            auto natts = static_cast<size_t>(desc.GetMaxAttrNumber());
            auto schema = mi::DatabaseGlobal->GetSchema();
            auto outputs = std::vector<mi::db::catalog::TypeInfo::OutputFunction>{natts};
            auto attrs = desc.Attributes();
            std::transform(attrs.begin(), attrs.end(), outputs.begin(),
                           [=](const mi::access::table::AttributeDescriptor &attr) {
                               auto &info = schema->GetTypeInfo(attr.TypeId());
                               return info.GetOutputFunction();
                           });
            this->_outputs = std::optional{outputs};
        }

        return this->_outputs.value();
    }

  public:
    SocketServer(int socket, WorkerId id) : _id(id), _client(socket) {}

    std::optional<CommandType> ReadNextCommand() {
        auto type = this->_client.ReceiveInt8Opt();
        if (!type.has_value()) {
            return std::nullopt;
        }

        switch (type.value()) {
        case 'S':
            return CommandType::SELECT;
        case 'I':
            return CommandType::INSERT;
        case 'U':
            return CommandType::UPDATE;
        case 'D':
            return CommandType::DELETE;
        case 'B':
            return CommandType::BEGIN;
        case 'C':
            return CommandType::COMMIT;
        case 'R':
            return CommandType::ROLLBACK;
        default:
            throw std::runtime_error("operation not supported: " + std::to_string(type.value()));
        }
    };

    int32_t ReadInt32() { return this->_client.ReceiveInt32(); }

    int16_t ReadInt16() { return this->_client.ReceiveInt16(); }

    void SendTupleDescriptor(const mi::access::table::TupleDescriptor &desc) {
        this->_client.SendInt8('D');

        auto natts = static_cast<size_t>(desc.GetMaxAttrNumber());
        this->_client.SendInt32(static_cast<int32_t>(natts));

        for (size_t i = 0; i < natts; i++) {
            auto &att = desc.Attributes()[i];
            if (att.ByVal()) {
                this->_client.SendInt32(att.Length());
            } else {
                this->_client.SendInt32(-1);
            }
        }
    }

    void SendTuple(const mi::access::table::TupleDescriptor &desc,
                   mi::access::table::ITuple &tuple) {
        // 'T'uple
        this->_client.SendInt8('T');

        // Attribute values
        auto maxAttno = desc.GetMaxAttrNumber();
        auto &outputs = this->getOutputFunctions(desc);
        for (AttrNumber attno = AttrNumber::Min; attno <= maxAttno; attno++) {
            auto datum = tuple.GetAttribute(attno);
            if (datum.has_value()) {
                this->_client.SendInt8('1');
                const auto &att = desc.Attributes()[attno.ToIndex()];
                if (att.ByVal()) {
                    switch (att.Length()) {
                    case 8:
                        this->_client.SendInt64(datum.value().getScalar<int64_t>());
                        break;
                    case 4:
                        this->_client.SendInt32(datum.value().getScalar<int32_t>());
                        break;
                    case 2:
                        this->_client.SendInt16(datum.value().getScalar<int16_t>());
                        break;
                    case 1:
                        this->_client.SendInt8(datum.value().getScalar<int8_t>());
                        break;
                    default:
                        throw std::runtime_error("unknown length");
                    }
                } else {
                    auto value = outputs[attno.ToIndex()](datum.value());
                    this->_client.SendString(value);
                }
            } else {
                this->_client.SendInt8('0');
            }
        }
    }

    void SendOk() {
        // 'O'k
        this->_client.SendInt8('O');
    }

    void SendStringResult(const std::string &message) {
        // 'S'tring
        this->_client.SendInt8('S');
        this->_client.SendString(message);
    }
};

// worker_state
thread_local mi::worker::Worker *mi::MyWorker;
thread_local mi::storage::trans::Transaction *mi::MyTransaction;

static void verify_transaction_ok() {
    if (mi::MyTransaction == nullptr) {
        throw std::runtime_error("There is no transaction");
    }
    if (mi::MyTransaction->GetStatus() != mi::storage::trans::TransactionStatus::RUNNING) {
        throw std::runtime_error("Invalid transaction status");
    }
}

static void handle_select(SocketServer &server) {
    verify_transaction_ok();

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);
    mi::MyTransaction->BeginNewStatement();

    auto scan = table->StartScan(mi::MyTransaction->GetSnapshot());

    scan->BeginScan();

    auto &desc = *table->GetDescriptor();
    server.SendTupleDescriptor(desc);

    while (auto tuple = scan->GetNextTuple()) {
        server.SendTuple(desc, *tuple);
    }

    scan->EndScan();
    server.SendOk();
}

static void handle_update(SocketServer &server) {
    verify_transaction_ok();

    auto val1 = server.ReadInt32();
    auto val2 = server.ReadInt16();
    auto newTuple = mi::executor::VirtualTuple{std::vector{mi::Datum{val1}, mi::Datum{val2}},
                                               std::vector{false, false}};

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::MainTableId);

    mi::MyTransaction->BeginNewStatement();

    auto scan = table->StartScan(mi::MyTransaction->GetSnapshot());

    scan->BeginScan();

    while (auto tuple = scan->GetNextTuple()) {
        table->UpdateTuple(*tuple, newTuple);
    }

    scan->EndScan();

    server.SendOk();
}

static void handle_delete(SocketServer &server) {
    verify_transaction_ok();

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);

    mi::MyTransaction->BeginNewStatement();

    auto scan = table->StartScan(mi::MyTransaction->GetSnapshot());

    scan->BeginScan();

    while (auto tuple = scan->GetNextTuple()) {
        table->DeleteTuple(*tuple);
    }

    scan->EndScan();

    server.SendOk();
}

static void handle_insert(SocketServer &server) {
    verify_transaction_ok();

    // Read tuple we want to insert.
    // For now only 1 table exists and schema is fixed - tuple is known.
    auto val1 = server.ReadInt32();
    auto val2 = server.ReadInt16();
    auto tuple = mi::executor::VirtualTuple{std::vector{mi::Datum{val1}, mi::Datum{val2}},
                                            std::vector{false, false}};

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);

    mi::MyTransaction->BeginNewStatement();

    table->InsertTuple(tuple);

    server.SendOk();
}

static void handle_begin(SocketServer &server) {
    if (mi::MyTransaction != nullptr) {
        throw std::runtime_error("Transaction already exists");
    }

    mi::MyTransaction = mi::TransactionManagerGlobal->BeginNewTransaction();
    server.SendOk();
}

static void handle_commit(SocketServer &server) {
    mi::TransactionManagerGlobal->CommitTransaction(mi::MyTransaction->GetXID());

    mi::MyTransaction = nullptr;
    server.SendOk();
}

static void rollback_state() {
    mi::TransactionManagerGlobal->AbortTransaction(mi::MyTransaction->GetXID());

    if (auto undoLog = mi::MyTransaction->GetUndoLogIfAny()) {
        undoLog->UndoAllRecords();
    }
}

static void handle_rollback(SocketServer &server) {
    if (mi::MyTransaction == nullptr) {
        throw std::runtime_error("There is no transaction");
    }

    rollback_state();

    delete mi::MyTransaction;
    mi::MyTransaction = nullptr;

    server.SendOk();
}

static void handle_loop(SocketServer &server, WorkerId id) {
    // Setup environment
    mi::MyWorker = mi::WorkerGlobal->GetWorker(id);

    // Handle connection itself
    while (auto command = server.ReadNextCommand()) {
        try {
            if (command == CommandType::BEGIN) {
                handle_begin(server);
            } else if (command == CommandType::COMMIT) {
                handle_commit(server);
            } else if (command == CommandType::ROLLBACK) {
                handle_rollback(server);
            } else if (command == CommandType::SELECT) {
                handle_select(server);
            } else if (command == CommandType::INSERT) {
                handle_insert(server);
            } else if (command == CommandType::UPDATE) {
                handle_update(server);
            } else if (command == CommandType::DELETE) {
                handle_delete(server);
            } else {
                server.SendStringResult("Only SELECT/INSERT/UPDATE/DELETE are supported for now");
            }
        } catch (std::exception &ex) {
            server.SendStringResult(std::string("ERROR: ") + ex.what());
            if (mi::MyTransaction != nullptr)
                mi::MyTransaction->SetStatus(mi::storage::trans::TransactionStatus::ABORTED);
        }
    }
}

void mi::worker::HandleUserConnection(WorkerId workerId, int sock) {
    auto server = SocketServer{sock, workerId};
    std::cerr << "starting processing client for worker " << workerId << std::endl;
    try {
        handle_loop(server, workerId);
    } catch (std::exception &ex) {
        std::cerr << "something went wrong: " << ex.what() << std::endl;
    }
}
