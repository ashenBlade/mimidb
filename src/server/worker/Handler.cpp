#include "worker/Handler.hpp"
#include "MimiClient.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "cluster_state.hpp"
#include "db/bulitin/int.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "executor/Datum.hpp"
#include "executor/VirtualTuple.hpp"
#include "executor/expr/FunctionExpressionNode.hpp"
#include "executor/expr/IExpressionNode.hpp"
#include "executor/func/FunctionContext.hpp"
#include "executor/plan/DeleteNode.hpp"
#include "executor/plan/InsertNode.hpp"
#include "executor/plan/SeqScan.hpp"
#include "executor/plan/UpdateNode.hpp"
#include "logger.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionManager.hpp"
#include "utils/DatumArray.hpp"
#include "worker/WorkerManager.hpp"
#include "worker_state.hpp"
#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

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
        for (auto attno = AttrNumber::Min(); attno <= maxAttno; ++attno) {
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

static std::unique_ptr<mi::executor::IExpressionNode> create_tuple_predicate(int32_t value) {
    // tuple.a = 1
    auto fctx = mi::executor::FunctionContext{mi::db::builtin::Int32Eq, true, 2};

    // value is on right side
    auto constants = mi::DatumArray{{mi::Datum{value}}, {false}};
    auto constantMapping = std::vector{1UL};

    // 'tuple.a' attribute is on left side
    auto attrMapping = std::vector{std::make_pair<AttrNumber, size_t>(AttrNumber::Min(), 0)};
    auto node = std::make_unique<mi::executor::FunctionExpressionNode>(
        std::move(fctx), std::move(constants), std::move(constantMapping), std::move(attrMapping));

    return node;
}

static void handle_select(SocketServer &server) {
    verify_transaction_ok();

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);
    mi::MyTransaction->BeginNewStatement();

    auto node = mi::executor::plan::SeqScan{table.get(), mi::MyTransaction->GetSnapshot(),
                                            create_tuple_predicate(2)};

    node.Start();

    auto &desc = *table->GetDescriptor();
    server.SendTupleDescriptor(desc);

    while (auto tuple = node.Execute()) {
        server.SendTuple(desc, *tuple);
    }

    node.End();
    server.SendOk();
}

static std::vector<std::pair<AttrNumber, std::unique_ptr<mi::executor::IExpressionNode>>>
create_tuple_update_expr() {
    // UPDATE tbl SET a = a + 1

    // a + 1
    auto fctx = mi::executor::FunctionContext{mi::db::builtin::Int32Add, true, 2};

    auto constants = mi::DatumArray{{mi::Datum{1}}, {false}};
    auto constantMapping = std::vector{1UL};

    // 'tbl.a' - is first attribute (AttrNumber::Min())
    auto attrMapping = std::vector{std::make_pair(AttrNumber::Min(), 0UL)};

    auto expr = std::make_unique<mi::executor::FunctionExpressionNode>(
        std::move(fctx), std::move(constants), std::move(constantMapping), std::move(attrMapping));

    auto vec = std::vector<std::pair<AttrNumber, std::unique_ptr<mi::executor::IExpressionNode>>>{};
    vec.emplace_back(AttrNumber::Min(), std::move(expr));
    return vec;
}

static void handle_update(SocketServer &server) {
    verify_transaction_ok();

    // auto val1 = server.ReadInt32();
    // auto val2 = server.ReadInt16();
    // auto newTuple = mi::executor::VirtualTuple{std::vector{mi::Datum{val1}, mi::Datum{val2}},
    //                                            std::vector{false, false}};

    // UPDATE tbl SET a = a + 1
    auto updateExpr = create_tuple_update_expr();
    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::MainTableId);
    auto qual = create_tuple_predicate(1);

    mi::MyTransaction->BeginNewStatement();

    auto node = mi::executor::plan::UpdateNode{
        table.get(), std::move(qual), mi::MyTransaction->GetSnapshot(), std::move(updateExpr)};

    node.Start();

    // Does not return anything
    (void)node.Execute();

    node.End();

    server.SendOk();
}

static void handle_delete(SocketServer &server) {
    verify_transaction_ok();

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);

    mi::MyTransaction->BeginNewStatement();

    auto qual = create_tuple_predicate(2);
    auto node = mi::executor::plan::DeleteNode{table.get(), std::move(qual),
                                               mi::MyTransaction->GetSnapshot()};

    node.Start();

    // Does not return anything
    (void)node.Execute();

    node.End();

    server.SendOk();
}

static void handle_insert(SocketServer &server) {
    verify_transaction_ok();

    // Read tuple we want to insert.
    // For now only 1 table exists and schema is fixed - tuple is known.
    auto val1 = server.ReadInt32();
    auto val2 = server.ReadInt16();
    std::unique_ptr<mi::access::table::ITuple> tuple = std::make_unique<mi::executor::VirtualTuple>(
        std::vector{mi::Datum{val1}, mi::Datum{val2}}, 
        std::vector{false, false});

    mi::MyTransaction->BeginNewStatement();

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);

    auto placeholder = std::vector<std::unique_ptr<mi::access::table::ITuple>>{};
    placeholder.emplace_back(std::move(tuple));
    auto node = mi::executor::plan::InsertNode{table.get(), std::move(placeholder)};

    node.Start();

    // Does not return anything
    (void)node.Execute();

    node.End();

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
        mi::LoggerGlobal->Debug("got command %i", command.value());
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
                mi::LoggerGlobal->Error("command %i is not supported", command.value());
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
    mi::LoggerGlobal->Info("starting processing client for worker %i", workerId.value);
    try {
        handle_loop(server, workerId);
    } catch (std::exception &ex) {
        mi::LoggerGlobal->Error("something went wrong: %s", ex.what());
    }
}
