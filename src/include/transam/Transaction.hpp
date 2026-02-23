#pragma once

#include <optional>

#include "transam/TransactionId.hpp"

namespace mi::transam {

class Transaction {
  private:
    /// @brief Assigned XID for this transaction
    std::optional<TransactionId> _xid;
    
  public:
};

}; // namespace mi::transam