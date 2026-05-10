#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <sys/types.h>

#include "storage/File.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/TransactionId.hpp"
#include "transam/UndoRecord.hpp"
#include "transam/UndoSeqNumber.hpp"
#include "utils/ByteArray.hpp"
#include "utils/NonCopyable.hpp"

namespace mi::transam {

class UndoLog : private NonCopyable {
  private:
    /// @brief Path to undo log file
    std::string _path;
    /// @brief Lock for write
    std::mutex _writeMutex;
    /// @brief Offset to end of undo log file at which to perform write
    off64_t _size;

    UndoLog(std::string path, off64_t size);
    
    UndoSeqNumber getCurrentUSN() const;
  public:

    /// @brief Insert new undo record
    UndoSeqNumber InsertUndoRecord(IUndoRecord &record);

    /// @brief Undo record at given location
    void PerformUndo(UndoSeqNumber usn);

    /// @brief Read record from undo loc for specified transaction and CSN
    /// @param xid Transaction Id to which record belong
    /// @param usn Number of target undo record
    /// @param length Out parameter to which total record length is saved (for debugging and assertions)
    /// @return Pointer to byte array of this record
    std::unique_ptr<IUndoRecord> GetRecord(UndoSeqNumber usn);
    
    static UndoLog *Open(std::string path);
};

}; // namespace mi::transam