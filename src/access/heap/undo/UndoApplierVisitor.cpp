#include "mimidb.hpp"

#include "access/heap/HeapPage.hpp"
#include "access/heap/ItemId.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "cluster_state.hpp"
#include "storage/BufferLock.hpp"
#include "storage/PageTag.hpp"

#include "access/heap/undo/UndoApplierVisitor.hpp"

using namespace mi::access::heap::undo;

void UndoApplierVisitor::Visit([[maybe_unused]] DeleteUndoRecord &record) {
    // Read page where to which this tuple belongs
    auto tag = storage::PageTag{record.TableId, record.TupId.pageno};
    auto pin = BufferPoolGlobal->GetBuffer(tag);

    // Detect whether this record is already undone
    WITH (auto lock = storage::BufferLock{pin.GetBuffer()}) {
        auto page = HeapPage{pin.GetContents()};
        auto &itemId = page.GetItemId(record.TupId.itemid);
        if (!itemId.isNormal()) {
            // Nothing to do
            return;
        }

        auto tuple = page.GetTuple(itemId);
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
}
