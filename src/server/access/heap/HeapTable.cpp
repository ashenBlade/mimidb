#include "access/heap/HeapTable.hpp"
#include "access/heap/HeapPage.hpp"
#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "access/heap/HeapTuple.hpp"
#include "access/heap/HeapTupleSerializer.hpp"
#include "access/heap/ItemId.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/heap/undo/InsertUndoRecord.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "access/heap/wal/DeleteHeapWALRecord.hpp"
#include "access/heap/wal/InsertHeapWALRecord.hpp"
#include "access/heap/wal/UpdateHeapWALRecord.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "cluster_state.hpp"
#include "mimidb.hpp"
#include "storage/buffer/BufferLock.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageNumber.hpp"
#include "storage/buffer/PageTag.hpp"
#include "storage/buffer/RelFile.hpp"
#include "storage/undo/UndoSeqNumber.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "trans/TransactionId.hpp"
#include "utils/BitUtils.hpp"
#include "worker_state.hpp"
#include <cstring>
#include <fcntl.h>
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
        storage::undo::UndoSeqNumber::Invalid,
        flags,
        dataStart,
    };
    return HeapPageTuple{header, std::move(values), std::move(isnull)};
}

// Search page on table with required free space or start new page.
// Returned buffer is already locked in S mode.
// Passed freeSpace is a total required size including item id and padding.
mi::storage::buffer::BufferPin HeapTable::searchPageFreeSpace(size_t freeSpace) const {
    auto file = storage::buffer::RelFile::Open(this->_tableId, O_RDONLY);
    auto npages = file.GetPagesCount();
    for (storage::buffer::PageNumber attno = storage::buffer::PageNumber::Min; attno < npages; ++attno) {
        auto tag = storage::buffer::PageTag{this->_tableId, attno};
        auto buffer = BufferPoolGlobal->GetBuffer(tag);
        auto page = HeapPage{buffer.GetContents()};
        auto lock = storage::buffer::BufferSharedLock{buffer.GetBuffer()};
        if (page.GetFreeSpace() < freeSpace) {
            continue;
        }

        return buffer;
    }

    // All pages are full - start new page
    auto buffer = BufferPoolGlobal->ExtendRelation(this->_tableId);
    auto page = HeapPage{buffer.GetContents()};
    if (page.IsNew())
        HeapPage::Init(page);
    auto lock = storage::buffer::BufferSharedLock{buffer.GetBuffer()};
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
        auto pin = this->searchPageFreeSpace(freeSpace);

        // update S -> X, so we can change page contents
        WITH(auto lock = storage::buffer::BufferLock{pin.GetBuffer()}) {
            auto page = HeapPage{pin.GetBuffer()->GetContents()};

            // Check we still have free space
            if (page.GetFreeSpace() < freeSpace) {
                continue;
            }

            // TupleId of newly inserted tuple (always last)
            auto tupleId = TupleId{pin.GetPageTag().PageNo, page.ItemsCount()};

            // First, create and insert undo record
            auto undoRec = undo::DeleteUndoRecord{this->_tableId, tupleId};
            auto usn = MyTransaction->GetUndoLog().InsertRecord(undoRec);

            // Now we have USN, so set it to tuple and actually insert
            heapPageTuple.Header().undo = usn;
            auto serializedTuple =
                HeapTupleSerializer::Serialize(heapPageTuple, *this->_tupleDescriptor, tupleSize);

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
            std::memcpy(ptr, serializedTuple.data(), tupleSize);

            // Update lower/upper
            header.lower += sizeof(ItemId);
            header.upper -= MaxAlign(tupleSize);

            inserted = true;
        }
    } while (!inserted);
}

static void wait_tnx_end(mi::storage::trans::TransactionId xid) {
    auto csn = mi::TransactionManagerGlobal->GetTransactionCsn(xid);
    // Even if tuple on page can be different does not mean we should abort right now.
    // Transaction updating tuple can abort and we continue successfully.
    // So obtain current TNX status and decide what to do next.
    if (csn.IsInProgress()) {
        mi::TransactionManagerGlobal->WaitTransactionEnd(xid);
        csn = mi::TransactionManagerGlobal->GetTransactionCsn(xid);
    }

    // For now we must know tnx status
    assert(!(csn.IsNormal() || csn.IsCommitting()));

    if (csn.IsAborted()) {
        throw std::runtime_error(
            "TNX aborted but for now I do not support undoing another transaction");
    } else if (csn.IsFrozen() || csn.IsNormal()) {
        throw std::runtime_error("tuple was concurrently modified");
    } else {
        assert(false);
        throw std::runtime_error("invalid status for transaction");
    }
}

void HeapTable::UpdateTuple(ITuple &oldTuple, ITuple &newTuple) {
    auto &oldHeapTuple = dynamic_cast<HeapTuple &>(oldTuple);

    auto newHeapPageTuple = this->formHeapPageTuple(newTuple);
    auto newTupSize = HeapTupleSerializer::CalculateSize(newHeapPageTuple, *this->_tupleDescriptor);

    auto oldTID = oldHeapTuple.GetTID();
    auto tag = storage::buffer::PageTag{this->_tableId, oldTID.pageno};
    auto oldPin = BufferPoolGlobal->GetBuffer(tag);
    auto lock = storage::buffer::BufferLock{oldPin.GetBuffer()};

    // First we must check that old tuple was not concurrently modified.
    // If so throw error - we can not proceed due to concurrent access.
    auto oldPage = HeapPage{oldPin.GetContents()};
    auto itemId = oldPage.GetItemId(oldTID.itemid);
    auto oldHeapPageTuple = oldPage.GetTuple(itemId);

    // For concurrent access it's enough to check only tuple XID, because if it was modified,
    // then tuple was modified for sure. Passed oldTuple must be obtained using table scan
    // so must be visible and tnx is committed. Other side is that tuple is being modified
    // by us, but for now there is no support for multistatement DML transaction.
    if (oldHeapPageTuple->xid != oldHeapTuple.GetHeader().xid) {
        wait_tnx_end(oldHeapPageTuple->xid);
        // For now should not get here
        assert(false);
    }

    // Now decide which path to take in order to update tuple
    auto newTID = oldTID;
    auto newPin = storage::buffer::BufferPin{};
    auto newLock = storage::buffer::BufferLock{};
    if (newTupSize <= itemId.getLength()) {
        // If new tuple fits old one, than we can successfully overwrite it.
    } else if (newTupSize + sizeof(ItemId) <= oldPage.GetFreeSpace()) {
        // There is enough space for new tuple on the same page.
        // Tuple will be inserted last on page.
        newTID.itemid = oldPage.ItemsCount();
    } else {
        // We must search new page for tuple
        while (true) {
            newPin = this->searchPageFreeSpace(newTupSize + sizeof(ItemId));
            newLock = storage::buffer::BufferLock{newPin.GetBuffer()};
            auto newPage = HeapPage{newPin.GetContents()};

            // There is a possibility, that before we pinned page someone perform concurrent update
            // and now we lack space
            if (newTupSize + sizeof(ItemId) <= newPage.GetFreeSpace()) {
                newTID = TupleId{newPin.GetPageTag().PageNo, newPage.ItemsCount()};
                break;
            }
        }
    }

    // We know where to place new tuple, so begin.
    // First undo record
    storage::undo::UndoSeqNumber usn;
    {
        auto size = HeapTupleSerializer::CalculateSize(oldHeapTuple.GetHeapPageTuple(),
                                                       *this->_tupleDescriptor);
        auto buffer = HeapTupleSerializer::Serialize(oldHeapTuple.GetHeapPageTuple(),
                                                     *this->_tupleDescriptor, size);
        auto record = undo::UpdateUndoRecord{this->_tableId, oldTID, newTID, buffer};
        usn = MyTransaction->GetUndoLog().InsertRecord(record);
    }

    // Now we have USN for this tuple - update it
    newHeapPageTuple.Header().undo = usn;
    auto size = HeapTupleSerializer::CalculateSize(newHeapPageTuple, *this->_tupleDescriptor);
    auto buffer = HeapTupleSerializer::Serialize(newHeapPageTuple, *this->_tupleDescriptor, size);

    // Then WAL record
    storage::wal::LogSeqNumber lsn;
    {
        auto record = heap::wal::UpdateHeapWALRecord{this->_tableId, newTID, oldTID, buffer};
        lsn = WALGlobal->WriteLogRecord(record);
    }

    // Now we are ready to update page contents
    if (newTID == oldTID) {
        // New tuple overwrites old one.
        // Note we do not change itemId's length because transaction can be aborted and we must be
        // able to undo changes (otherwise someone can concurrently insert new data there)
        auto &itemId = oldPage.GetItemId(newTID.itemid);
        auto header = oldPage.GetTuple(itemId);
        std::memcpy(reinterpret_cast<std::byte *>(header), buffer.data(), buffer.size());
    } else if (newTID.pageno == oldTID.pageno) {
        // Tuples on the same page, but different places
        auto &itemId = oldPage.GetItemId(newTID.itemid);
        auto &header = oldPage.GetHeader();
        auto offset = static_cast<uint16_t>(header.upper - buffer.size());

        // Add new ItemId
        itemId.setNormal(size, offset);
        header.lower += sizeof(ItemId);

        // Insert tuple
        std::memcpy(reinterpret_cast<std::byte *>(oldPage.GetTuple(itemId)), buffer.data(),
                    buffer.size());
    } else {
        // New tuple is on different page
        auto newPage = HeapPage{newPin.GetContents()};
        auto &itemId = newPage.GetItemId(newTID.itemid);
        auto &header = newPage.GetHeader();
        auto offset = static_cast<uint16_t>(header.upper - buffer.size());

        // Add new ItemId
        itemId.setNormal(size, offset);
        header.lower += sizeof(ItemId);

        // Insert tuple
        std::memcpy(reinterpret_cast<std::byte *>(newPage.GetTuple(itemId)), buffer.data(),
                    buffer.size());
    }

    // Mark old tuple as updated
    if (newTID != oldTID) {
        auto itemId = oldPage.GetItemId(oldTID.itemid);
        auto header = oldPage.GetTuple(itemId);
        header->flags = static_cast<HeapTupleFlags>(header->flags | HeapTupleFlags::Deleted);
        header->xid = MyTransaction->GetXID();
        header->undo = usn;
        header->dataStartOffset = 0;

        // Do not change ItemId's length, because if transaction aborts, then we must be able to
        // undo this update. If we do not have space we can not undo.
    }

    // Do not forget to update LSN on page after we have done WAL insert
    oldPage.GetHeader().lsn = lsn;
    if (newTID.pageno != oldTID.pageno) {
        auto newPage = HeapPage{newPin.GetContents()};
        newPage.GetHeader().lsn = lsn;
    }

    // Mark page dirty to spill it to disk
    oldPin.GetBuffer()->MarkDirty();
    if (newPin.IsValid()) {
        newPin.GetBuffer()->MarkDirty();
    }
}

void HeapTable::DeleteTuple(ITuple &tuple) {
    auto &heapTuple = dynamic_cast<HeapTuple &>(tuple);
    auto tid = heapTuple.GetTID();

    auto pin = BufferPoolGlobal->GetBuffer(storage::buffer::PageTag{this->_tableId, tid.pageno});
    auto lock = storage::buffer::BufferLock{pin.GetBuffer()};
    auto page = HeapPage{pin.GetContents()};

    auto &itemId = page.GetItemId(heapTuple.GetTID().itemid);
    auto tuplePageHeader = page.GetTuple(itemId);
    auto &heapPageTuple = heapTuple.GetHeapPageTuple();

    if (tuplePageHeader->xid != heapPageTuple.Header().xid) {
        wait_tnx_end(tuplePageHeader->xid);
        // For now, should not get here
        assert(false);
    }

    auto xid = MyTransaction->GetXID();
    auto newTupleHeader =
        HeapPageTupleHeader{xid, storage::undo::UndoSeqNumber::Invalid, HeapTupleFlags::Deleted, 0};

    // Create undo record and save old tuple
    auto oldTupleSer = std::vector<std::byte>{itemId.getLength()};
    std::memcpy(oldTupleSer.data(), tuplePageHeader, itemId.getLength());
    auto undoRecord = undo::InsertUndoRecord{this->_tableId, tid, std::move(oldTupleSer)};
    auto usn = MyTransaction->GetUndoLog().InsertRecord(undoRecord);

    // Update USN in tuple header
    newTupleHeader.undo = usn;

    // Create WAL record and save new tuple
    auto walRecord = wal::DeleteHeapWALRecord{this->_tableId, tid};
    auto lsn = WALGlobal->WriteLogRecord(walRecord);

    // For now we have only to update header and do not touch tuple contents
    std::memcpy(tuplePageHeader, reinterpret_cast<std::byte *>(&newTupleHeader),
                sizeof(HeapPageTupleHeader));

    // Update page LSN
    page.GetHeader().lsn = lsn;
}

std::unique_ptr<mi::access::table::ITableScan>
HeapTable::StartScan(storage::trans::Snapshot *snapshot) {
    return std::make_unique<HeapTableScan>(snapshot, this);
}

// Return number of pages for given relation. If
mi::storage::buffer::PageNumber HeapTable::GetPageCount() {
    auto file = storage::buffer::RelFile::Open(this->_tableId, O_RDONLY);
    return file.GetPagesCount();
}
