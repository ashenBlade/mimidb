#pragma once

#include <memory>

#include "transam/TransactionId.hpp"
#include "transam/VirtualUndoLog.hpp"

namespace mi::transam {

class UndoLog {
  private:
    /// @brief File descriptor for underlying undo log
    int _fd;
  public:
    
    /// @brief Open new Virtual Undo Log for given xid
    /// @param xid XID which uses this log
    /// @return 
    std::unique_ptr<VirtualUndoLog> OpenVirtualUndoLog(TransactionId xid);
};

}; // namespace mi::transam