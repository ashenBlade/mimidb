#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/undo/InsertUndoRecord.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "mimidb.hpp"

#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <unistd.h>

#include "access/heap/HeapPage.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "access/heap/HeapTuple.hpp"
#include "storage/BufferLock.hpp"
#include "storage/PageNumber.hpp"

#include "cluster_state.hpp"
#include "transam/CommitSeqNumber.hpp"
#include "transam/Snapshot.hpp"
#include "transam/TransactionId.hpp"

using namespace mi::access::heap;

HeapTableScan::HeapTableScan(std::shared_ptr<mi::transam::Snapshot> snapshot,
                             HeapTable &table)
    : _tupleId(), _lastPageNumber(), _snapshot(snapshot), _table(table) {}

void HeapTableScan::BeginScan() {
    this->_lastPageNumber = this->_table.GetPageCount();
    assert(this->_lastPageNumber.IsValid());
    // Start iterating from first page. Note that BufferWrapper is initialized with Invalid, so we
    // will read buffer at first scan call
    this->_tupleId = TupleId{storage::PageNumber::Min, 0};

    // If relation is empty, then no scan is needed
    if (this->_lastPageNumber == 0) {
        this->_end = true;
    }
};

// Create new HeapTuple from given on page tuple to return from scan node
static std::unique_ptr<HeapTuple> build_heap_tuple(std::shared_ptr<mi::access::table::TupleDescriptor> descr, HeapPageTupleHeader *header, uint32_t length) {
    assert(length > sizeof(HeapPageTupleHeader));

    // Allocate new buffer and copy tuple contents there
    auto buffer = std::aligned_alloc(sizeof(void *), length);
    std::memcpy(buffer, reinterpret_cast<void *>(header), length);
    auto ptr = std::unique_ptr<HeapPageTupleHeader>(reinterpret_cast<HeapPageTupleHeader *>(buffer));

    return std::make_unique<HeapTuple>(descr, std::move(ptr), length);
}

static bool tuple_is_visible(const mi::transam::Snapshot &snapshot, HeapPageTupleHeader *header) {
    auto csn = mi::TransactionManagerGlobal->GetTransactionCsn(header->xid);
    
    assert(!csn.IsInvalid());

    if (csn.IsInProgress() || csn.IsAborted() || csn.IsCommitting()) {
        // XXX: not sure if committing means we do not see tuple
        return false;
    }

    if (csn.IsFrozen() || snapshot.IsVisibleFor(csn)) {
        return true;
    }

    return false;
}

static std::unique_ptr<mi::access::heap::HeapTuple> find_visible_tuple_page(std::shared_ptr<mi::access::table::TupleDescriptor> descr, HeapPageTupleHeader *header, const mi::transam::Snapshot &snapshot) {
    size_t length;
    auto xid = header->xid;
    auto usn = header->undo;
    do {
        auto record = mi::UndoLogGlobal->GetRecord<undo::HeapUndoRecordBase>(xid, usn, length);
        switch (record->RecordType) {
            case undo::HeapUndoRecordType::Insert: {
                auto insert = dynamic_cast<undo::InsertUndoRecord *>(record.get());
                if (tuple_is_visible(snapshot, &insert->Tuple)) {
                    return build_heap_tuple(descr, &insert->Tuple, insert->TupleLen);
                }

                xid = insert->Tuple.xid;
                usn = insert->Tuple.undo;
                break;
            }
            case undo::HeapUndoRecordType::Update: {
                auto update = dynamic_cast<undo::UpdateUndoRecord *>(record.get());
                if (update->From != update->To) {
                    return nullptr;
                }

                if (tuple_is_visible(snapshot, &update->Tuple)) {
                    return build_heap_tuple(descr,&update->Tuple, update->TupleLen);
                }

                xid = update->Tuple.xid;
                usn = update->Tuple.undo;
                break;
            }
            case undo::HeapUndoRecordType::Delete:
                // Tuple does not exist anymore
                return nullptr;
            default:
                throw std::runtime_error("unknown heap undo record type");
        }

        if (!usn.IsValid()) {
            return nullptr;
        }
    } while (true);
}

std::unique_ptr<mi::access::table::ITuple> HeapTableScan::GetNextTuple() {
    if (this->_end) {
        return nullptr;
    }

    // Read current page
    auto pagetag = storage::PageTag{this->_table.GetOid(), this->_tupleId.pageno};
    auto buffer = BufferPoolGlobal->GetBuffer(pagetag);
    auto lock = storage::BufferSharedLock{buffer.GetBuffer()};

    // Page is read and locked. We can read it's tuples now
    auto page = HeapPage{buffer.GetContents()};
    std::unique_ptr<HeapTuple> tuple = nullptr;
    auto index = this->_tupleId.itemid + 1;
    for (; index < page.ItemsCount(); ++index) {
        auto itemid = page.GetItemId(index);
        if (!itemid.isNormal()) {
            continue;
        }

        assert(itemid.hasHeader());

        auto header = page.GetTuple(itemid);
        
        if (tuple_is_visible(*this->_snapshot, header)) {
            if (header->flags & HeapTupleFlags::Deleted) {
                continue;
            }

            tuple = build_heap_tuple(this->_table.GetDescriptor(), header, itemid.getLength());
        } else {
            tuple = find_visible_tuple_page(this->_table.GetDescriptor(), header, *this->_snapshot);
        }

        if (tuple) {
            break;
        }
    }

    // Before returning update tuple id, so on next invocation we will search next tuple
    if (page.ItemsCount() <= index) {
        auto newPageno = pagetag.PageNo + 1;
        this->_tupleId = TupleId{newPageno, 0};
        if (this->_lastPageNumber <= newPageno) {
            this->_end = true;
        }
    } else {
        this->_tupleId.itemid = static_cast<uint16_t>(index + 1);
    }

    return tuple;
}

void mi::access::heap::HeapTableScan::EndScan() {
    this->_end = true;
    this->_tupleId = TupleId{};
}
