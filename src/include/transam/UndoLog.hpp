#pragma once

#include <cstddef>
#include <memory>

#include "storage/File.hpp"
#include "transam/TransactionId.hpp"
#include "transam/UndoSeqNumber.hpp"
#include "utils/ByteArray.hpp"

namespace mi::transam {

class UndoLog {
  public:
    /// @brief Read record from undo loc for specified transaction and CSN
    /// @param xid Transaction Id to which record belong
    /// @param usn Number of target undo record
    /// @param length Out parameter to which total record length is saved (for debugging and assertions)
    /// @return Pointer to byte array of this record
    std::unique_ptr<std::byte[]> GetRecordRaw(TransactionId xid, UndoSeqNumber usn, size_t &length);

    /// @brief Typed version of std::byte *GetRecord
    template<class T>
    std::unique_ptr<T> GetRecord(TransactionId xid, UndoSeqNumber usn, size_t &length) {
      auto value = this->GetRecordRaw(xid, usn, length);
      auto ptr = value.release();
      return std::unique_ptr<T>(reinterpret_cast<T *>(ptr));
    }
};

}; // namespace mi::transam