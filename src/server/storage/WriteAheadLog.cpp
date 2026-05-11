#include "storage/wal/WriteAheadLog.hpp"
#include "storage/io/File.hpp"
#include "storage/wal/IWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "storage/wal/WALRecordHeader.hpp"
#include "worker_state.hpp"
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <mutex>

using namespace mi::storage::wal;

WriteAheadLog::WriteAheadLog(std::string path, off64_t size, io::File file)
    : _path(path), _file(std::move(file)), _size(size) {};

LogSeqNumber WriteAheadLog::WriteLogRecord(const IWalRecord &record) {
    auto xid = MyTransaction->GetXID();
    auto header = WALRecordHeader{xid, record.GetRMgrId(), record.CalculateSize()};

    auto guard = std::lock_guard{this->_lock};

    auto offset = this->_size;
    auto lsn = LogSeqNumber{static_cast<uint64_t>(this->_size) + 1};
    this->_file.Write(reinterpret_cast<std::byte *>(&header), sizeof(WALRecordHeader), offset);
    offset += static_cast<off64_t>(sizeof(WALRecordHeader));

    auto buffer = std::vector<std::byte>{header.Length};
    record.Serialize(buffer.data());
    this->_file.Write(buffer.data(), buffer.size(), offset);
    offset += static_cast<off64_t>(buffer.size());

    this->_file.Fsync();
    this->_size = offset;

    return lsn;
}

WriteAheadLog *WriteAheadLog::Open(std::string path) {
    auto file = storage::io::File::Open(path, O_RDWR | O_CREAT);
    auto size = file.Size();
    return new WriteAheadLog{path, size, std::move(file)};
}
