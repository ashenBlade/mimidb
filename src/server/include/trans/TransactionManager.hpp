#pragma once

#include "trans/CommitSeqNumber.hpp"
#include "trans/Transaction.hpp"
#include "trans/TransactionId.hpp"
#include "utils/NonCopyable.hpp"
#include "worker/WorkerId.hpp"
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace mi::storage::trans {
class TransactionManager : private NonCopyable {
  private:
    /// @brief Current CSN value (last assigned)
    std::atomic<CommitSeqNumber::type> _csn;
    /// @brief Current XID value (last assigned)
    std::atomic<TransactionId::type> _xid;

    /// @brief Mutex to synchronize access to fields
    std::shared_mutex _mutex{};
    /// @brief Array of transaction states for each worker
    /// Note: it uses shared_ptr, because some fields can be used after transaction ends.
    /// i.e. LWLatch to wait for transaction end.
    std::vector<std::shared_ptr<Transaction>> _transactions;
    /// @brief Mapping between XID and it's 
    std::unordered_map<TransactionId, worker::WorkerId> _xidToWorker{};
    /// @brief Mapping between XID and it's CSN.
    std::unordered_map<TransactionId, CommitSeqNumber> _history{};

  public:
    TransactionManager(std::size_t workersCount);

    /// @brief For given transaction id return it's associated CSN
    CommitSeqNumber GetTransactionCsn(TransactionId xid);

    /// @brief Get current CSN value for cluster
    CommitSeqNumber GetCurrentCSN() const;

    /// @brief Get current XID value for cluster
    TransactionId GetCurrentXID() const;

    /// @brief Begin new transaction. This also will setup MyTransaction global variable
    void BeginNewTransaction();

    /// @brief Commit running transaction
    void CommitTransaction();

    /// @brief Abort current transaction
    void AbortTransaction();

    /// @brief Wait for this transaction to end.
    /// Either commit or abort.
    void WaitTransactionEnd(TransactionId xid);
};
} // namespace mi::transam