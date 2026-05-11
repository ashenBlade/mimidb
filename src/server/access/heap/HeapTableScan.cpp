#include "mimidb.hpp"

#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/HeapTupleSerializer.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/undo/InsertUndoRecord.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "access/table/TupleDescriptor.hpp"

#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <unistd.h>

#include "access/heap/HeapPage.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "access/heap/HeapTuple.hpp"
#include "storage/buffer/BufferLock.hpp"
#include "storage/buffer/PageNumber.hpp"

#include "cluster_state.hpp"
#include "trans/CommitSeqNumber.hpp"
#include "trans/ResourceManagerId.hpp"
#include "trans/Snapshot.hpp"
#include "trans/TransactionId.hpp"

using namespace mi::access::heap;

HeapTableScan::HeapTableScan(mi::transam::Snapshot *snapshot, HeapTable *table)
    : _snapshot(snapshot), _table(table), _end(false) {}

void HeapTableScan::BeginScan() {
    this->_lastPageNumber = this->_table->GetPageCount();
    assert(this->_lastPageNumber.IsValid());
    // Start iterating from first page. Note that BufferWrapper is initialized with Invalid, so we
    // will read buffer at first scan call
    this->_tupleId = TupleId{storage::PageNumber::Min, 0};

    // If relation is empty, then no scan is needed
    if (this->_lastPageNumber == 0U) {
        this->_end = true;
    }
};

// Create new HeapTuple from given on page tuple to return from scan node
static std::unique_ptr<HeapTuple> build_heap_tuple(const mi::access::table::TupleDescriptor *descr,
                                                   HeapPageTupleHeader *header, TupleId tid) {
    auto tuple =
        HeapTupleSerializer::Deserialize(reinterpret_cast<const std::byte *>(header), *descr);
    return std::make_unique<HeapTuple>(descr, std::move(tuple), tid);
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

static std::unique_ptr<HeapTuple> find_visible_tuple_page(HeapPageTupleHeader *header,
                                                          const mi::access::table::TupleDescriptor *descriptor,
                                                          mi::transam::Snapshot &snapshot) {
    auto usn = header->undo;
    std::unique_ptr<HeapTuple> tuple = nullptr;
    while (usn.IsValid()) {
        auto record = mi::UndoLogGlobal->GetRecord(usn);
        if (record->GetRMgrId() != mi::transam::ResourceManagerId::Heap) {
            throw std::runtime_error("invalid rmgrid");
        }

        switch (static_cast<undo::HeapUndoRecordType>(record->GetType())) {
        case undo::HeapUndoRecordType::Delete:
            // Tuple does not exist anymore
            break;
        case undo::HeapUndoRecordType::Update: {
            auto updateRecord = dynamic_cast<undo::UpdateUndoRecord *>(record.get());

            // We find visible version only at the same location, because if tuple is not visible, then
            // we can find another visible version during scan.
            if (updateRecord->NewLocation != updateRecord->OldLocation) {
                return nullptr;
            }

            auto tuple = reinterpret_cast<HeapPageTupleHeader *>(updateRecord->TupleData.data());
            
            auto csn = mi::TransactionManagerGlobal->GetTransactionCsn(tuple->xid);
            assert(!csn.IsCommitting());

            if (csn.IsNormal() || csn.IsFrozen()) {
                if (snapshot.IsVisibleFor(csn)) {
                    return build_heap_tuple(descriptor, tuple, updateRecord->NewLocation);
                }
            }

            usn = tuple->undo;
        }
        case undo::HeapUndoRecordType::Insert: {
            auto insertRecord = dynamic_cast<undo::InsertUndoRecord *>(record.get());

            auto tuple = reinterpret_cast<HeapPageTupleHeader *>(insertRecord->TupleData.data());
            auto csn = mi::TransactionManagerGlobal->GetTransactionCsn(tuple->xid);
            assert(!csn.IsCommitting());

            if (csn.IsNormal() || csn.IsFrozen()) {
                if (snapshot.IsVisibleFor(csn)) {
                    return build_heap_tuple(descriptor, tuple, insertRecord->Location);
                }
            }

            usn = tuple->undo;
        }
        default:
            throw std::runtime_error("unknown heap undo record type");
        }
    }

    return tuple;
}

std::unique_ptr<mi::access::table::ITuple> HeapTableScan::GetNextTuple() {
    if (this->_end) {
        return nullptr;
    }

    // Read current page
    auto pagetag = storage::PageTag{this->_table->GetOid(), this->_tupleId.pageno};
    auto pin = BufferPoolGlobal->GetBuffer(pagetag);
    auto lock = storage::BufferSharedLock{pin.GetBuffer()};

    // Page is read and locked. We can read it's tuples now
    auto page = HeapPage{pin.GetContents()};
    std::unique_ptr<HeapTuple> tuple = nullptr;
    auto index = this->_tupleId.itemid;
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

            tuple = build_heap_tuple(this->_table->GetDescriptor(), header,
                                     TupleId{pagetag.PageNo, index});
        } else {
            tuple = find_visible_tuple_page(header, this->_table->GetDescriptor(), *this->_snapshot);
        }

        if (tuple) {
            break;
        }
    }

    // Before returning update tuple id, so on next invocation we will search next tuple
    if (page.ItemsCount() <= index) {
        auto newPageno = pagetag.PageNo + 1U;
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
