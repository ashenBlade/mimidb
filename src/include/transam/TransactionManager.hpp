#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "transam/CommitSeqNumber.hpp"
#include "transam/Transaction.hpp"
#include "transam/TransactionId.hpp"
#include "utils/NonCopyable.hpp"

namespace mi::transam {
class TransactionManager : private NonCopyable {
    private:
        /// @brief Mutex to synchronize access to fields
        std::shared_mutex _mutex;
        /// @brief Current CSN value
        std::atomic<CommitSeqNumber::type> _csn;
        /// @brief Current XID value
        std::atomic<TransactionId::type> _xid;
        /// @brief Mapping between transaction and it's status
        std::unordered_map<TransactionId, CommitSeqNumber> _status;

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
        TransactionId BeginNewTransaction();
        
        /// @brief Commit existing transaction
        /// @return Assigned CSN for transaction
        CommitSeqNumber CommitTransaction(TransactionId xid);

        /// @brief Mark transaction as aborted
        void AbortTransaction(TransactionId xid);
};
} // namespace mi::transam