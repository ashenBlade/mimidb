#pragma once

#include <cstddef>
#include <memory>

#include "storage/File.hpp"
#include "transam/TransactionId.hpp"
#include "transam/VirtualUndoLog.hpp"

namespace mi::transam {

class UndoLog {
  private:
    /// @brief File descriptor for underlying undo log
    storage::File _file;
  public:

    /// @brief Open new Virtual Undo Log for given xid
    /// @param xid XID which uses this log
    /// @return 
    std::unique_ptr<VirtualUndoLog> OpenVirtualUndoLog(TransactionId xid);

    /// @brief Read record from undo loc for specified transaction and CSN
    /// @param xid Transaction Id to which record belong
    /// @param usn Number of target undo record
    /// @param length Out parameter to which total record length is saved (for debugging and assertions)
    /// @return Pointer to byte array of this record
    std::unique_ptr<std::byte> GetRecordRaw(TransactionId xid, UndoSeqNumber usn, size_t &length);

    /// @brief Typed version of std::byte *GetRecord
    template<class T>
    std::unique_ptr<T> GetRecord(TransactionId xid, UndoSeqNumber usn, size_t &length) {
      return std::static_pointer_cast<T>(this->GetRecordRaw(xid, usn, length));
    }
};

}; // namespace mi::transam