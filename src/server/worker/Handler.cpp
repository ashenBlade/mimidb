#include "worker/Handler.hpp"
#include "MimiClient.hpp"
#include "SQLParser.h"
#include "SQLParserResult.h"
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
#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/IPacket.hpp"
#include "packets/PacketType.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include "parser/ParserError.hpp"
#include "parser/SQLParser.hpp"
#include "planner/Planner.hpp"
#include "sql/SQLStatement.h"
#include "sql/TransactionStatement.h"
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

    std::unique_ptr<mi::interface::libmimi::IPacket> ReadNextPacket() {
        return this->_client.ReceivePacket();
    }

    void SendTupleDescriptor([[maybe_unused]] const mi::access::table::TupleDescriptor &desc) {
        // Пока только 1 таблица, поэтому ок
        auto attrs = std::vector<mi::interface::libmimi::AttributeDescription>{
            mi::interface::libmimi::AttributeDescription{"a"},
            mi::interface::libmimi::AttributeDescription{"b"},
        };
        auto packet = mi::interface::libmimi::TupleDescriptionPacket{std::move(attrs)};
        this->_client.SendPacket(packet);
    }

    void SendTuple(const mi::access::table::TupleDescriptor &desc,
                   mi::access::table::ITuple &tuple) {
        auto maxAttno = desc.GetMaxAttrNumber();
        auto &outputs = this->getOutputFunctions(desc);
        auto attrs = std::vector<std::optional<std::string>>{};
        for (auto attno = AttrNumber::Min(); attno <= maxAttno; ++attno) {
            auto datum = tuple.GetAttribute(attno);
            if (datum.has_value()) {
                auto value = outputs[attno.ToIndex()](datum.value());
                attrs.emplace_back(std::move(value));
            } else {
                attrs.emplace_back(std::nullopt);
            }
        }

        auto packet = mi::interface::libmimi::DataRowPacket{std::move(attrs)};
        this->_client.SendPacket(packet);
    }

    void SendCommandComplete() {
        auto packet = mi::interface::libmimi::CommandCompletePacket{};
        this->_client.SendPacket(packet);
    }

    void SendError(const std::string &message) {
        auto packet = mi::interface::libmimi::ErrorResponsePacket{message};
        this->_client.SendPacket(packet);
    }
};

// worker_state
thread_local mi::worker::Worker *mi::MyWorker;
thread_local mi::storage::trans::Transaction *mi::MyTransaction;

[[maybe_unused]] static void verify_transaction_ok() {
    if (mi::MyTransaction == nullptr) {
        throw std::runtime_error("There is no transaction");
    }
    if (mi::MyTransaction->GetStatus() != mi::storage::trans::TransactionStatus::RUNNING) {
        throw std::runtime_error("Invalid transaction status");
    }
}

static void start_new_transaction_command() {
    if (mi::MyTransaction != nullptr) {
        // Ok, BEGIN; can be called multiple times
        return;
    }

    mi::MyTransaction = mi::TransactionManagerGlobal->BeginNewTransaction();
}

static void handle_begin(SocketServer &server) {
    start_new_transaction_command();
    server.SendCommandComplete();
}

static void commit_transaction_command() {
    mi::TransactionManagerGlobal->CommitTransaction(mi::MyTransaction->GetXID());
    mi::MyTransaction = nullptr;
}

static void handle_commit(SocketServer &server) {
    commit_transaction_command();
    server.SendCommandComplete();
}

static void rollback_state() {
    mi::TransactionManagerGlobal->AbortTransaction(mi::MyTransaction->GetXID());

    if (auto undoLog = mi::MyTransaction->GetUndoLogIfAny()) {
        undoLog->UndoAllRecords();
    }
}

static void abort_transaction_command() {
    if (mi::MyTransaction == nullptr) {
        throw std::runtime_error("There is no transaction");
    }
    
    rollback_state();
    mi::MyTransaction = nullptr;
}

static void handle_rollback(SocketServer &server) {
    abort_transaction_command();
    server.SendCommandComplete();
}

static void exec_tcl_query(SocketServer &server, hsql::SQLStatement &statement) {
    hsql::TransactionStatement &stmt = dynamic_cast<hsql::TransactionStatement &>(statement);
    switch (stmt.command) {
    case hsql::TransactionCommand::kBeginTransaction:
        handle_begin(server);
        break;
    case hsql::TransactionCommand::kRollbackTransaction:
        handle_rollback(server);
        break;
    case hsql::TransactionCommand::kCommitTransaction:
        handle_commit(server);
        break;
    }
}

class ImplicitTransactionGuard {
  private:
    bool _inImplicitTnx;

  public:
    ImplicitTransactionGuard() {
        auto tnxIsRunning = mi::MyTransaction != nullptr;
        this->_inImplicitTnx = !tnxIsRunning;
        if (tnxIsRunning) { 
            return;
        }

        start_new_transaction_command();
    }

    void Commit() {
        if (!this->_inImplicitTnx) {
            return;
        }

        commit_transaction_command();
        this->_inImplicitTnx = false;
    }
    
    void Abort() {
        if (!this->_inImplicitTnx) {
            return;
        }

        abort_transaction_command();
        this->_inImplicitTnx = false;
    }

    ~ImplicitTransactionGuard() {
        // This is called at the end, if exception was thrown - we must cleanup state
        // so user will not have to send abort manually
        this->Abort();
    }
};

static void exec_plannable_query(SocketServer &server, hsql::SQLStatement &statement) {
    auto node = mi::planner::Planner::Plan(statement);

    // Start implicit transaction if not started one yet
    auto tnxBlock = ImplicitTransactionGuard{};

    // Begin new statement
    mi::MyTransaction->BeginNewStatement();
    auto snapshot = mi::MyTransaction->GetSnapshot();

    // Пока я знаю, что только 1 таблица есть, поэтому не надо возиться с дескриптором
    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);
    const auto &descriptor = *table->GetDescriptor();
    server.SendTupleDescriptor(descriptor);

    node->Start(snapshot);

    while (auto tuple = node->Execute()) {
        server.SendTuple(descriptor, *tuple);
    }

    node->End();

    server.SendCommandComplete();

    tnxBlock.Commit();
}

static void handle_loop(SocketServer &server, WorkerId id) {
    // Setup environment
    mi::MyWorker = mi::WorkerGlobal->GetWorker(id);

    // Handle connection itself
    while (auto packet = server.ReadNextPacket()) {
        mi::LoggerGlobal->Debug("got command %i", packet->Type());
        if (packet->Type() != mi::interface::libmimi::PacketType::Query) {
            server.SendError("Packet type " + std::to_string(static_cast<int>(packet->Type())) +
                             " is not supported");
            continue;
        }

        auto queryPacket = dynamic_cast<mi::interface::libmimi::QueryPacket *>(packet.get());

        mi::LoggerGlobal->Debug("got query: %s", queryPacket->Query().c_str());
        
        // Only 1 types of statements are supported: TCL and simple SQL crud
        try {
            auto statement = mi::parser::SQLParser::ParseStatement(queryPacket->Query());
            if (statement->type() == hsql::kStmtTransaction) {
                exec_tcl_query(server, *statement);
            } else if (mi::planner::Planner::IsPlannableStatement(*statement)) {
                exec_plannable_query(server, *statement);
            } else {
                server.SendError("Statement " +
                                 std::to_string(static_cast<int>(statement->type())) +
                                 " is not supported");
            }
        } catch (std::exception &ex) {
            server.SendError(ex.what());
            mi::LoggerGlobal->Error("could not execute query \"%s\": %s", queryPacket->Query().c_str(), ex.what());

            // All errors abort transaction (if exists)
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
