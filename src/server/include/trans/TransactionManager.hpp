#pragma once

#include "trans/CommitSeqNumber.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionId.hpp"
#include "utils/NonCopyable.hpp"
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace mi::storage::trans {
class TransactionManager : private NonCopyable {
  private:
    /// @brief Mutex to synchronize access to fields
    std::shared_mutex _mutex;
    /// @brief Current CSN value (last assigned)
    std::atomic<CommitSeqNumber::type> _csn;
    /// @brief Current XID value (last assigned)
    std::atomic<TransactionId::type> _xid;
    /// @brief Storage for all active transactions
    std::unordered_map<TransactionId, std::unique_ptr<Transaction>> _state;
    /// @brief Mapping between XID and it's CSN
    std::unordered_map<TransactionId, CommitSeqNumber> _history;

  public:
    explicit TransactionManager();

    /// @brief For given transaction id return it's associated CSN
    CommitSeqNumber GetTransactionCsn(TransactionId xid);

    /// @brief Get current CSN value for cluster
    CommitSeqNumber GetCurrentCSN() const;

    /// @brief Get current XID value for cluster
    TransactionId GetCurrentXID() const;

    /// @brief Begin new transaction
    /// @return Assigned XID for transaction
    Transaction *BeginNewTransaction();

    /// @brief Commit existing transaction
    /// @return Assigned CSN for transaction
    CommitSeqNumber CommitTransaction(TransactionId xid);

    /// @brief Mark transaction as aborted
    void AbortTransaction(TransactionId xid);

    /// @brief Wait for this transaction to end.
    /// Either commit or abort.
    void WaitTransactionEnd(TransactionId xid);
};
} // namespace mi::transam