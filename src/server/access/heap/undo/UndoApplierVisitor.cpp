#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "mimidb.hpp"

#include "access/heap/HeapPage.hpp"
#include "access/heap/ItemId.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "cluster_state.hpp"
#include "storage/buffer/BufferLock.hpp"
#include "storage/buffer/PageTag.hpp"
#include <cstring>
#include <stdexcept>

#include "access/heap/undo/UndoApplierVisitor.hpp"

using namespace mi::access::heap::undo;

void UndoApplierVisitor::Visit(DeleteUndoRecord &record) {
    // Read page where to which this tuple belongs
    auto tag = storage::PageTag{record.TableId, record.TupId.pageno};
    auto pin = BufferPoolGlobal->GetBuffer(tag);

    // Detect whether this record is already undone
    auto lock = storage::BufferLock{pin.GetBuffer()};
    auto page = HeapPage{pin.GetContents()};
    auto &itemId = page.GetItemId(record.TupId.itemid);
    if (!itemId.isNormal()) {
        // Nothing to do
        return;
    }

    auto tuple = page.GetTuple(itemId);
    // TODO: надо еще XID проверять, это передавать при вызове IResourceManager
    if (tuple->undo != this->_usn) {
        // Tuple was changed - nothing to do
        return;
    }

    // Actually we do not have to remove or somewhat clean page contents.
    // Mark ItemId dead is enough to just skip entry
    itemId.setDead();

    // Finally, mark page dirty
    pin.GetBuffer()->MarkDirty();
}

void UndoApplierVisitor::Visit(UpdateUndoRecord &record) {
    if (record.NewLocation == record.OldLocation) {
        auto pin = BufferPoolGlobal->GetBuffer(storage::PageTag{record.TableId, record.OldLocation.pageno});
        auto lock = storage::BufferLock{pin.GetBuffer()};
        auto page = HeapPage{pin.GetContents()};
        auto itemId = page.GetItemId(record.OldLocation.itemid);
        auto currentTuple = page.GetTuple(itemId);
        auto oldTuple = reinterpret_cast<HeapPageTupleHeader *>(record.TupleData.data());
        
        // Check this undo record was already applied
        if (oldTuple == currentTuple) {
            return;
        }

        // Someone concurrently updated tuple. This is invalid, since nobody can do it - protocol violation.
        if (currentTuple->undo != this->_usn) {
            throw std::runtime_error("can not undo update: tuple was concurrently modified");
        }

        // Actually rewrite tuple to old data. Do not touch ItemId since it wasn't modified.
        std::memcpy(reinterpret_cast<std::byte *>(currentTuple), record.TupleData.data(), record.TupleData.size());
    } else if (record.NewLocation.pageno == record.OldLocation.pageno) {
        // New tuple placed on the same page
        auto pin = BufferPoolGlobal->GetBuffer(storage::PageTag{record.TableId, record.OldLocation.pageno});
        auto lock = storage::BufferLock{pin.GetBuffer()};
        auto page = HeapPage{pin.GetContents()};
        auto oldItemId = page.GetItemId(record.OldLocation.itemid);
        auto currentOldTuple = page.GetTuple(oldItemId);
        auto oldTuple = reinterpret_cast<HeapPageTupleHeader *>(record.TupleData.data());

        // Check this undo record was already applied
        if (oldTuple == currentOldTuple) {
            return;
        }

        // Someone concurrently updated tuple. This is invalid, since nobody can do it - protocol violation.
        if (currentOldTuple->undo != this->_usn) {
            throw std::runtime_error("can not undo update: old tuple was concurrently modified");
        }
        
        auto newItemId = page.GetItemId(record.NewLocation.itemid);
        auto currentNewTuple = page.GetTuple(newItemId);
        if (currentNewTuple->undo != this->_usn) {
            throw std::runtime_error("can not undo update: new tuple was concurrently modified");
        }

        // We have verified data was not changed - apply undo

        // Actually rewrite tuple to old data. Do not touch ItemId since it wasn't modified.
        std::memcpy(reinterpret_cast<std::byte *>(currentOldTuple), record.TupleData.data(), record.TupleData.size());

        // Mark new tuple as dead, so noone can access it
        newItemId.setDead();
    } else {
        // New tuple placed on DIFFERENT page
        auto oldPin = BufferPoolGlobal->GetBuffer(storage::PageTag{record.TableId, record.OldLocation.pageno});
        auto oldLock = storage::BufferLock{oldPin.GetBuffer()};
        auto oldPage = HeapPage{oldPin.GetContents()};
        auto oldItemId = oldPage.GetItemId(record.OldLocation.itemid);
        auto currentOldTuple = oldPage.GetTuple(oldItemId);
        auto oldTuple = reinterpret_cast<HeapPageTupleHeader *>(record.TupleData.data());

        // Check this undo record was already applied
        if (oldTuple == currentOldTuple) {
            return;
        }

        // Someone concurrently updated tuple. This is invalid, since nobody can do it - protocol violation.
        if (currentOldTuple->undo != this->_usn) {
            throw std::runtime_error("can not undo update: old tuple was concurrently modified");
        }

        // Now verify new page
        auto newPin = BufferPoolGlobal->GetBuffer(storage::PageTag{record.TableId, record.NewLocation.pageno});
        auto newLock = storage::BufferLock{newPin.GetBuffer()};
        auto newPage = HeapPage{newPin.GetContents()};
        auto &newItemId = newPage.GetItemId(record.NewLocation.itemid);
        auto currentNewTuple = newPage.GetTuple(newItemId);
        if (currentNewTuple->undo != this->_usn) {
            throw std::runtime_error("can not undo update: new tuple was concurrently modified");
        }

        // We have verified data was not changed - apply undo

        // Actually rewrite tuple to old data. Do not touch ItemId since it wasn't modified.
        std::memcpy(reinterpret_cast<std::byte *>(currentOldTuple), record.TupleData.data(), record.TupleData.size());

        // Mark new tuple as dead, so noone can access it
        newItemId.setDead();
    }
}

void UndoApplierVisitor::Visit(InsertUndoRecord &record) {
    auto pin = BufferPoolGlobal->GetBuffer(storage::PageTag{record.TableId, record.Location.pageno});
    auto lock = storage::BufferLock{pin.GetBuffer()};
    auto page = HeapPage{pin.GetContents()};

    auto itemId = page.GetItemId(record.Location.itemid);
    auto tupleHeader = page.GetTuple(itemId);

    if (tupleHeader->undo != this->_usn) {
        // undo was already applied
        return;
    }

    // Update tuple contents
    assert(record.TupleData.size() <= itemId.getLength());
    std::memcpy(tupleHeader, record.TupleData.data(), record.TupleData.size());
}
