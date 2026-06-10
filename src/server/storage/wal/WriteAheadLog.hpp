#pragma once

#include "lock/LWLatch.hpp"
#include "storage/io/File.hpp"
#include "storage/wal/IRMgrWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include <mutex>
#include <string>

namespace mi::storage::wal {

class WriteAheadLog {
  private:
    /// Path to WAL file
    std::string _path;
    /// File object for underlying WAL
    io::File _file;
    /// File size
    uint64_t _end;
    // File position up to which it is flushed to disk.
    uint64_t _flushed;
    /// Latch for writing new entries
    lock::LWLatch _latch;

    WriteAheadLog(std::string path, uint64_t size, io::File _file);

  public:
    // Write single record to WAL file.
    // Returns LSN at which record ends.
    LogSeqNumber WriteLogRecord(const IRMgrWalRecord &record);

    // Flush and sync WAL file to disk up to given point.
    void Flush(LogSeqNumber upto);

    static WriteAheadLog *Open(std::string path);
};

}; // namespace mi::transam
