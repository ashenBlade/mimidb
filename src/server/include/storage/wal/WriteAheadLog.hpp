#pragma once

#include "storage/io/File.hpp"
#include "storage/wal/IWalRecord.hpp"
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
    off64_t _size;
    /// Lock for writing new entries
    std::mutex _lock;

    WriteAheadLog(std::string path, off64_t size, io::File _file);

  public:
    /// @brief Durable write record to WAL
    /// @return LSN at which record is written
    LogSeqNumber WriteLogRecord(const IWalRecord &record);

    static WriteAheadLog *Open(std::string path);
};

}; // namespace mi::transam
