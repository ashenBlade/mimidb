#include "storage/undo/UndoLog.hpp"
#include "cluster_state.hpp"
#include "storage/io/File.hpp"
#include "storage/undo/IUndoRecord.hpp"
#include "storage/undo/UndoLogRecordHeader.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "trans/TransactionId.hpp"
#include "utils/BitUtils.hpp"
#include "worker_state.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

using namespace mi::transam;

UndoLog::UndoLog(std::string path, off64_t size) : _path(path), _size(size) {};

UndoLog *UndoLog::Open(std::string path) {
    auto file = storage::File::Open(path, O_RDONLY | O_CREAT);
    auto size = file.Size();
    return new UndoLog{path, size};
}

static UndoLogRecordHeader read_header(mi::storage::File &file, off64_t offset) {
    UndoLogRecordHeader header;
    auto read =
        file.Read(reinterpret_cast<std::byte *>(&header), sizeof(UndoLogRecordHeader), offset);
    if (read != sizeof(UndoLogRecordHeader)) {
        throw std::runtime_error("could not read UndoLogRecordHeader");
    }

    return header;
}

std::unique_ptr<IUndoRecord> UndoLog::GetRecord(UndoSeqNumber usn) {
    assert(usn.IsValid());

    // Открываем UndoLog файл
    auto file = storage::File::Open(this->_path, O_RDONLY);

    // Читаем заголовок записи (получаем ее длину)
    auto offset = usn.value - 1;
    auto header = read_header(file, offset);
    auto &manager = RMgrRegistryGlobal->GetManager(header.ResourceManager);

    // Читаем оставшуюся запись
    auto buffer = std::vector<std::byte>(header.DataLength);
    offset += static_cast<int64_t>(sizeof(UndoLogRecordHeader));
    auto ret = file.Read(buffer.data(), header.DataLength, offset);
    if (ret != header.DataLength) {
        throw std::runtime_error("could not read UndoLogRecordHeader");
    }

    return manager.ParseUndo(header.RecordType, buffer.data(), buffer.size());
}

static std::vector<std::byte> format_undo_record(TransactionId xid, IUndoRecord &record) {
    // Align data
    auto size = record.CalculateSize();
    auto fullSize = sizeof(UndoLogRecordHeader) + mi::MaxAlign(size);
    auto buffer = std::vector<std::byte>(fullSize);
    auto header = UndoLogRecordHeader{xid, record.GetRMgrId(), record.GetType(), size};

    auto cursor = buffer.data();

    // Write header
    *reinterpret_cast<UndoLogRecordHeader *>(cursor) = header;
    cursor += sizeof(UndoLogRecordHeader);

    // And data itself
    record.Serialize(cursor);
    return buffer;
}

UndoSeqNumber UndoLog::getCurrentUSN() const {
    // USN always points to next byte after first
    return UndoSeqNumber{this->_size + 1};
}

UndoSeqNumber UndoLog::InsertUndoRecord(IUndoRecord &record) {
    auto file = storage::File::Open(this->_path, O_WRONLY);
    auto xid = MyTransaction->GetXID();

    auto buffer = format_undo_record(xid, record);
    auto guard = std::lock_guard{this->_writeMutex};

    // Получили лок - делаем запись
    file.Write(buffer.data(), buffer.size(), this->_size);
    file.Fsync();

    // Обновляем конец файла и рассчитываем USN
    auto usn = this->getCurrentUSN();
    this->_size += static_cast<off64_t>(buffer.size());
    return usn;
}

void UndoLog::PerformUndo(UndoSeqNumber usn) {
    auto file = storage::File::Open(this->_path, O_RDONLY);
    auto offset = usn.value - 1;

    // Read header and get associated RMgr
    auto header = read_header(file, offset);
    auto &manager = RMgrRegistryGlobal->GetManager(header.ResourceManager);

    // Read record payload
    auto payload = std::make_unique<std::byte[]>(header.DataLength);
    offset += static_cast<int64_t>(sizeof(UndoLogRecordHeader));
    auto ret = file.Read(payload.get(), header.DataLength, offset);
    if (ret != header.DataLength) {
        throw std::runtime_error("could not read UndoLogRecordHeader");
    }
    auto record = manager.ParseUndo(header.RecordType, payload.get(), header.DataLength);
    manager.ApplyUndo(*record, usn);
}
