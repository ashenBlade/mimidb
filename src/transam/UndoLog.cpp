#include "mimidb.hpp"
#include "storage/File.hpp"

#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "transam/UndoLog.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/TransactionId.hpp"
#include "transam/UndoLogRecordHeader.hpp"
#include "transam/UndoSeqNumber.hpp"
#include "utils/BitUtils.hpp"
#include "worker_state.hpp"

using namespace mi::transam;

UndoLog::UndoLog(std::string path, off64_t size): _path(path), _size(size) {};

UndoLog *UndoLog::Open(std::string path) {
    auto file = storage::File::Open(path, O_RDONLY | O_CREAT);
    auto size = file.Size();
    return new UndoLog{path, size};
}

std::unique_ptr<std::byte[]> UndoLog::GetRecordRaw([[maybe_unused]] TransactionId xid,
                                                   UndoSeqNumber usn, size_t &length) {
    assert(usn.IsValid());

    // Открываем UndoLog файл
    auto file = storage::File::Open(this->_path, O_RDONLY);

    // Читаем заголовок записи (получаем ее длину)
    auto header = UndoLogRecordHeader{};

    auto offset = usn.value - 1;
    auto read = file.Read(reinterpret_cast<std::byte *>(&header), sizeof(UndoLogRecordHeader), offset);
    if (read != sizeof(UndoLogRecordHeader)) {
        throw std::runtime_error("could not read UndoLogRecordHeader");
    }

    // Читаем оставшуюся запись
    auto buffer = std::make_unique<std::byte[]>(header.DataLength);
    offset += static_cast<int64_t>(sizeof(UndoLogRecordHeader));
    read = file.Read(buffer.get(), header.DataLength, offset);
    if (read != header.DataLength) {
        throw std::runtime_error("could not read UndoLogRecordHeader");
    }

    length = header.DataLength;
    return buffer;
}

static std::vector<std::byte> format_undo_record(TransactionId xid, ResourceManagerId rmgrid, std::byte *data, size_t size) {
    // Align data
    auto fullSize = sizeof(UndoLogRecordHeader) + mi::MaxAlign(size);
    auto buffer = std::vector<std::byte>(fullSize);
    auto header = UndoLogRecordHeader{xid, size, rmgrid};

    auto cursor = buffer.data();

    // Write header
    *reinterpret_cast<UndoLogRecordHeader *>(cursor) = header;
    cursor += sizeof(UndoLogRecordHeader);

    // And data itself
    std::memcpy(cursor, data, size);
    return buffer;
}

UndoSeqNumber UndoLog::getCurrentUSN() const {
    // USN always points to next byte after first
    return UndoSeqNumber{this->_size + 1};
}

UndoSeqNumber UndoLog::InsertUndoRecord(ResourceManagerId rmgrid, std::byte *data, size_t size) {
    auto file = storage::File::Open(this->_path, O_WRONLY);
    auto xid = MyTransaction->GetXID();
    auto vector = format_undo_record(xid, rmgrid, data, size);

    auto guard = std::lock_guard{this->_writeMutex};

    // Получили лок - делаем запись
    file.Write(vector.data(), vector.size(), this->_size);
    file.Fsync();

    // Обновляем конец файла и рассчитываем USN
    auto usn = this->getCurrentUSN();
    this->_size += static_cast<off64_t>(vector.size());
    return usn;
}
