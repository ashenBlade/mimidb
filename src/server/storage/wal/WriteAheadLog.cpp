#include "storage/wal/WriteAheadLog.hpp"
#include "storage/io/File.hpp"
#include "storage/wal/IRMgrWalRecord.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "storage/wal/WALRecordHeader.hpp"
#include "worker_state.hpp"
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <mutex>

using namespace mi::storage::wal;

WriteAheadLog::WriteAheadLog(std::string path, uint64_t size, io::File file)
    : _path(path), _file(std::move(file)), _end(size), _flushed(size) {};

LogSeqNumber WriteAheadLog::WriteLogRecord(const IRMgrWalRecord &record) {
    auto xid = MyTransaction->GetXID();
    auto header = WALRecordHeader{xid, record.GetRMgrId(), record.CalculateSize()};
    auto buffer = std::vector<std::byte>{header.Length};
    record.Serialize(buffer.data());

    auto l = std::unique_lock{this->_latch};

    auto offset = this->_end;
    this->_file.Write(reinterpret_cast<std::byte *>(&header), sizeof(WALRecordHeader),
    static_cast<off64_t>(offset));
    offset += sizeof(WALRecordHeader);

    this->_file.Write(buffer.data(), buffer.size(), static_cast<off64_t>(offset));
    offset += buffer.size();

    this->_end = offset;

    auto lsn = LogSeqNumber{this->_end + 1};
    return lsn;
}

void WriteAheadLog::Flush(LogSeqNumber lsn) {
    assert(lsn.IsValid());

    auto pos = lsn.value - 1;
    auto l = std::unique_lock{this->_latch};
    if (this->_flushed <= pos) {
        return;
    }

    this->_file.Fsync();
    this->_flushed = this->_end;
}

WriteAheadLog *WriteAheadLog::Open(std::string path) {
    auto file = storage::io::File::Open(path, O_RDWR | O_CREAT);
    auto size = static_cast<uint64_t>(file.Size());
    return new WriteAheadLog{path, size, std::move(file)};
}
