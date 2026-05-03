#include "access/heap/HeapPage.hpp"
#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/HeapTupleSerializer.hpp"
#include "access/heap/ItemId.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/heap/wal/InsertHeapWALRecord.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "cluster_state.hpp"
#include "mimidb.hpp"

#include "access/heap/HeapTable.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "storage/BufferLock.hpp"
#include "storage/BufferPin.hpp"
#include "storage/PageNumber.hpp"
#include "storage/PageTag.hpp"
#include "storage/RelFile.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/UndoLog.hpp"
#include "transam/UndoSeqNumber.hpp"
#include "utils/BitUtils.hpp"
#include "worker_state.hpp"

#include <cstring>
#include <fcntl.h>
#include <memory>
#include <stdexcept>

using namespace mi::access::heap;

HeapTable::HeapTable(Oid tableId, const TupleDescriptor *descriptor)
    : _tableId(tableId), _tupleDescriptor(descriptor) {}

HeapPageTuple HeapTable::formHeapPageTuple(mi::access::table::ITuple &tuple) const {
    auto maxAttr = this->_tupleDescriptor->GetMaxAttrNumber();
    auto values = std::vector<Datum>(maxAttr);
    auto isnull = std::vector<bool>(maxAttr);
    auto anyNull = false;
    for (table::AttrNumber attno = table::AttrNumber::Min; attno <= maxAttr; attno++) {
        auto datum = tuple.GetAttribute(attno);
        if (datum.has_value()) {
            values[attno.ToIndex()] = datum.value();
        } else {
            isnull[attno.ToIndex()] = true;
            anyNull = true;
        }
    }

    HeapTupleFlags flags;
    uint8_t dataStart;
    if (anyNull) {
        flags = HeapTupleFlags::HasNulls;

        uint32_t bitmapSize =
            (this->_tupleDescriptor->GetMaxAttrNumber().value + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        dataStart = static_cast<uint8_t>(sizeof(HeapPageTupleHeader) + bitmapSize);
        dataStart = MaxAlign(dataStart);
    } else {
        flags = static_cast<HeapTupleFlags>(0);
        dataStart = sizeof(HeapPageTupleHeader);
    }

    auto header = HeapPageTupleHeader{
        MyTransaction->GetXID(),
        transam::UndoSeqNumber::Invalid,
        flags,
        dataStart,
    };
    return HeapPageTuple{header, std::move(values), std::move(isnull)};
}

// Search page on table with required free space or start new page.
// Returned buffer is already locked in S mode.
// Passed freeSpace is a total required size including item id and padding.
mi::storage::BufferPin HeapTable::searchPageFreeSpace(size_t freeSpace) const {
    auto file = storage::RelFile::Open(this->_tableId, O_RDONLY);
    auto npages = file.GetPagesCount();
    for (storage::PageNumber attno = storage::PageNumber::Min; attno < npages; ++attno) {
        auto tag = storage::PageTag{this->_tableId, attno};
        auto buffer = BufferPoolGlobal->GetBuffer(tag);
        auto page = HeapPage{buffer.GetContents()};
        auto lock = storage::BufferSharedLock{buffer.GetBuffer()};
        if (page.GetFreeSpace() < freeSpace) {
            continue;
        }

        return buffer;
    }

    // All pages are full - start new page
    auto buffer = BufferPoolGlobal->ExtendRelation(this->_tableId);
    auto page = HeapPage{buffer.GetContents()};
    auto lock = storage::BufferSharedLock{buffer.GetBuffer()};
    if (page.GetFreeSpace() < freeSpace) {
        throw std::runtime_error("all pages are occupied");
    }

    return buffer;
}
void HeapTable::InsertTuple(ITuple &tuple) {
    // Form HeapPageTuple
    auto heapPageTuple = this->formHeapPageTuple(tuple);
    auto tupleSize = HeapTupleSerializer::CalculateSize(heapPageTuple, *this->_tupleDescriptor);

    // Search page with required free space - tuple and it's itemid.
    auto freeSpace = tupleSize + sizeof(ItemId);
    auto inserted = false;
    do {
        auto buffer = this->searchPageFreeSpace(freeSpace);

        // update S -> X, so we can change page contents
        WITH (auto lock = storage::BufferLock{buffer.GetBuffer()}) {
            auto page = HeapPage{buffer.GetBuffer()->GetContents()};

            // Check we still have free space
            if (page.GetFreeSpace() < freeSpace) {
                continue;
            }

            // TupleId of newly inserted tuple (always last)
            auto tupleId = TupleId{buffer.GetPageTag().PageNo, page.ItemsCount()};

            // First, create and insert undo record
            auto undoRec = undo::DeleteUndoRecord{tupleId};
            auto usn = UndoLogGlobal->InsertUndoRecord(transam::ResourceManagerId::Heap,
                                                       reinterpret_cast<std::byte *>(&undoRec),
                                                       sizeof(undo::DeleteUndoRecord));

            // Now we have USN, so set it to tuple and actually insert
            heapPageTuple.Header().undo = usn;
            auto serializedTuple =
                HeapTupleSerializer::Serialize(heapPageTuple, *this->_tupleDescriptor, static_cast<ssize_t>(tupleSize));

            // Make WAL entry before writing to disk
            auto walRec = wal::InsertHeapWALRecord{tupleId, serializedTuple};
            auto lsn = WALGlobal->WriteLogRecord(walRec);
            
            // Now perform changes itself
            auto &header = page.GetHeader();
            header.lsn = lsn;
            auto lp = page.GetLinePointerArray();
            auto &itemId = lp[page.ItemsCount()];

            // Update it's ItemId
            auto offset = static_cast<uint16_t>(page.GetHeader().upper - MaxAlign(tupleSize));
            itemId.flags = ItemState::Normal;
            itemId.setLength(static_cast<uint16_t>(tupleSize));
            itemId.setOffset(offset);

            // Insert tuple itself
            auto ptr = reinterpret_cast<std::byte *>(page.GetTuple(itemId));
            std::memcpy(ptr, serializedTuple->data(), tupleSize);

            // Update lower/upper
            header.lower += sizeof(ItemId);
            header.upper -= MaxAlign(tupleSize);

            inserted = true;
        }
    } while (!inserted);
}

void HeapTable::UpdateTuple([[maybe_unused]] std::shared_ptr<ITuple> oldTuple,
                            [[maybe_unused]] std::shared_ptr<ITuple> newTuple) {
    throw std::runtime_error("not implemented");
}

std::unique_ptr<mi::access::table::ITableScan>
HeapTable::StartScan(std::shared_ptr<mi::transam::Snapshot> snapshot) {
    return std::make_unique<HeapTableScan>(snapshot, this);
}

void HeapTable::DeleteTuple([[maybe_unused]] std::shared_ptr<ITuple> tuple) {
    throw std::runtime_error("not implemented");
}

// Return number of pages for given relation. If
mi::storage::PageNumber HeapTable::GetPageCount() {
    auto file = storage::RelFile::Open(this->_tableId, O_RDONLY);
    return file.GetPagesCount();
}
