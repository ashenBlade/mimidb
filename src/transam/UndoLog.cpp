#include "mimidb.hpp"
#include "storage/File.hpp"

#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <stdexcept>

#include "transam/UndoLog.hpp"
#include "transam/UndoLogRecordHeader.hpp"

using namespace mi::transam;

static std::string get_undo_log_path() { return std::string{"undolog"}; }

std::unique_ptr<std::byte[]> UndoLog::GetRecordRaw([[maybe_unused]] TransactionId xid,
                                                   UndoSeqNumber usn, size_t &length) {
    assert(usn.IsValid());

    // Открываем UndoLog файл
    auto file = storage::File::Open(get_undo_log_path(), O_RDONLY | O_CREAT);

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
