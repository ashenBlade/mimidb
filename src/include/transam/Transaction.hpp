#pragma once

#include <optional>

#include "transam/Snapshot.hpp"
#include "transam/TransactionId.hpp"

namespace mi::transam {
enum TransactionStatus {
    IDLE = 0, // Not executing, waiting for statement to execute
    RUNNING = 1, // Executing statement
    
};

class Transaction {
  private:
    /// @brief Assigned XID for this transaction
    TransactionId _xid;
    /// @brief Snapshot for this transaction
    Snapshot _snapshot;

  public:
    Transaction(TransactionId xid, Snapshot snapshot) : _xid(xid), _snapshot(snapshot) {}
    TransactionId GetXID() const { return this->_xid; }
    Snapshot GetSnapshot() const { return this->_snapshot; }
};

}; // namespace mi::transam