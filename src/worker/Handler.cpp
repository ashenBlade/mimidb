#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "db/catalog/TableId.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "executor/VirtualTuple.hpp"
#include "mimidb.hpp"

#include "access/heap/HeapTable.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "cluster_state.hpp"
#include "transam/Snapshot.hpp"
#include "transam/TransactionManager.hpp"
#include "worker/WorkerManager.hpp"
#include "worker_state.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <vector>

#include "worker/Handler.hpp"

using namespace mi::worker;
using AttrNumber = mi::access::table::AttrNumber;

enum CommandType {
    /* Start from 1 to use as condition in 'while' loop */
    SELECT = 1,
    INSERT = 2,
    UPDATE = 3,
    DELETE = 4,
};

class SocketServer {
  private:
    WorkerId _id;
    int _socket;

    // Vector of output functions for each attribute
    std::optional<std::vector<mi::db::catalog::TypeInfo::OutputFunction>> _outputs;

    const std::vector<mi::db::catalog::TypeInfo::OutputFunction> &
    getOutputFunctions(const mi::access::table::TupleDescriptor &desc) {
        if (!this->_outputs.has_value()) {
            auto natts = desc.GetMaxAttrNumber().ToIndex();
            auto schema = mi::DatabaseGlobal->GetSchema();
            auto outputs = std::vector<mi::db::catalog::TypeInfo::OutputFunction>{natts};
            auto attrs = desc.Attributes();
            std::transform(attrs.begin(), attrs.end(), outputs.begin(),
                           [&schema](const mi::access::table::AttributeDescriptor &attr) {
                               auto &info = schema->GetTypeInfo(attr.TypeId());
                               return info.GetOutputFunction();
                           });
            this->_outputs = std::optional{outputs};
        }

        return this->_outputs.value();
    }

    void sendByte(char value) {
        auto ret = send(this->_socket, &value, sizeof(value), 0);
        if (ret < 0) {
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "could not send");
        }
    }

    void sendInt32(int32_t value) {
        auto v = static_cast<uint32_t>(value);
        v = htonl(v);
        auto ret = send(this->_socket, &v, sizeof(v), 0);
        if (ret < 0) {
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "could not send");
        }
    }

    void sendString(const std::string &value) {
        this->sendInt32(static_cast<int32_t>(value.size()));

        auto buf = value.c_str();
        auto left = value.size();
        while (left > 0) {
            auto ret = send(this->_socket, buf, left, 0);
            if (ret < 0) {
                throw std::system_error(std::error_code(errno, std::system_category()),
                                        "could not send");
            }

            left -= static_cast<size_t>(ret);
            buf += ret;
        }
    }

  public:
    SocketServer(int socket, WorkerId id) : _id(id), _socket(socket) {}

    std::optional<CommandType> ReadNextCommand() {
        char type;
        auto ret = recv(this->_socket, &type, sizeof(char), 0);
        if (ret < 0) {
            std::cerr << "could not recv: " << strerror(errno) << std::endl;
            return std::nullopt;
        }

        if (ret == 0) {
            return std::nullopt;
        }

        assert(ret == 1);
        switch (type) {
        case 'S':
            return CommandType::SELECT;
        case 'I':
            return CommandType::INSERT;
        case 'U':
            return CommandType::UPDATE;
        case 'D':
            return CommandType::DELETE;
        default:
            throw std::runtime_error("operation not supported: " + std::to_string(type));
        }
    };

    int32_t ReadInt32() {
        int32_t value;

        auto ret = recv(this->_socket, &value, sizeof(int32_t), 0);
        if (ret < 0) {
            std::cerr << "could not recv: " << strerror(errno) << std::endl;
            throw std::runtime_error("error reading int32");
        }

        return static_cast<int32_t>(ntohl(static_cast<uint32_t>(value)));
    }

    int16_t ReadInt16() {
        int16_t value;

        auto ret = recv(this->_socket, &value, sizeof(int16_t), 0);
        if (ret < 0) {
            std::cerr << "could not recv: " << strerror(errno) << std::endl;
            throw std::runtime_error("error reading int32");
        }

        return static_cast<int16_t>(ntohs(static_cast<uint16_t>(value)));
    }

    void SendTupleDescriptor(const mi::access::table::TupleDescriptor &desc) {
        this->sendByte('D');
        this->sendInt32(static_cast<int32_t>(desc.GetMaxAttrNumber().ToIndex()));
    }

    void SendTuple(const mi::access::table::TupleDescriptor &desc,
                   mi::access::table::ITuple &tuple) {
        // 'T'uple
        this->sendByte('T');

        // Attribute values
        auto maxAttno = desc.GetMaxAttrNumber();
        auto &outputs = this->getOutputFunctions(desc);
        for (AttrNumber attno = AttrNumber::Min; attno < maxAttno; attno++) {
            auto datum = tuple.GetAttribute(attno);
            if (datum.has_value()) {
                this->sendByte('1');
                auto value = outputs[attno.ToIndex()](datum.value());
                this->sendString(value);
            } else {
                this->sendByte('0');
            }
        }
    }

    void SendEnd() {
        // 'E'nd
        this->sendByte('E');
    }

    void SendStringResult(const std::string &message) {
        auto buf = message.c_str();
        auto left = message.size();

        // 'S'tring
        auto type = 'S';
        if (send(this->_socket, &type, sizeof(char), 0) != 1) {
            throw std::runtime_error("could not send 'S' message type");
        }

        auto length = htonl(static_cast<uint32_t>(left));
        if (send(this->_socket, &length, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
            throw std::runtime_error("could not send message length");
        }

        while (left > 0) {
            auto ret = send(this->_socket, buf, left, 0);
            if (ret < 0) {
                throw std::runtime_error("could not send data: " + std::string(strerror(errno)));
            }

            if (ret == 0) {
                // Next recv return EOF
                return;
            }

            buf += ret;
            left -= static_cast<size_t>(ret);
        }
    }

    ~SocketServer() {
        if (this->_socket == -1) {
            return;
        }

        /* Close socket */
        shutdown(this->_socket, SHUT_RDWR);
        close(this->_socket);
        this->_socket = -1;
    }
};

static void handle_select(SocketServer &server) {
    auto xid = mi::TransactionManagerGlobal->BeginNewTransaction();
    std::cerr << "Beginning new tnx: xid = " << xid << std::endl;

    auto csn = mi::TransactionManagerGlobal->GetCurrentCSN();
    auto snapshot = std::make_shared<mi::transam::Snapshot>(csn);

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);
    auto scan = std::make_shared<mi::access::heap::HeapTableScan>(
        snapshot, dynamic_cast<mi::access::heap::HeapTable *>(table.get()));

    scan->BeginScan();

    auto &desc = *table->GetDescriptor();
    server.SendTupleDescriptor(desc);

    while (auto tuple = scan->GetNextTuple()) {
        server.SendTuple(desc, *tuple);
    }

    scan->EndScan();
    server.SendEnd();

    csn = mi::TransactionManagerGlobal->CommitTransaction(xid);
    std::cerr << "Commit CSN = " << csn << std::endl;
}

static void handle_insert(SocketServer &server) {
    // Read tuple we want to insert.
    // For now only 1 table exists and schema is fixed - tuple is known.
    auto val1 = server.ReadInt32();
    auto val2 = server.ReadInt16();
    auto tuple = mi::executor::VirtualTuple{std::vector{mi::Datum{val1}, mi::Datum{val2}},
                                            std::vector{false, false}};

    // Beginning new transaction
    auto xid = mi::TransactionManagerGlobal->BeginNewTransaction();
    auto csn = mi::TransactionManagerGlobal->GetCurrentCSN();

    // For now snapshot is not required
    [[maybe_unused]] auto snapshot = std::make_shared<mi::transam::Snapshot>(csn);

    auto table = mi::DatabaseGlobal->OpenTable(mi::schema::catalog::TableId::MainTableId);

    table->InsertTuple(tuple);

    csn = mi::TransactionManagerGlobal->CommitTransaction(xid);
    std::cerr << "Commit CSN = " << csn << std::endl;
}

static void handle_loop(SocketServer &server, WorkerId id) {
    // Setup environment
    mi::MyWorker = mi::WorkerGlobal->GetWorker(id);

    // Handle connection itself
    while (auto command = server.ReadNextCommand()) {
        if (command == CommandType::SELECT) {
            handle_select(server);
        } else if (command == CommandType::INSERT) {
            handle_insert(server);
        } else {
            server.SendStringResult("Only SELECT is supported for now");
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
