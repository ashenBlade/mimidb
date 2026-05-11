#pragma once

#include "access/heap/undo/IHeapUndoRecordVisitor.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"
#include "storage/undo/UndoSeqNumber.hpp"

namespace mi::access::heap::undo {
class UndoApplierVisitor : public IHeapUndoRecordVisitor {
  private:
    // UndoSeqNumber for record to apply
    storage::undo::UndoSeqNumber _usn;
  public:
    UndoApplierVisitor(storage::undo::UndoSeqNumber usn) : _usn(usn) {};
    void Visit(DeleteUndoRecord &record) override;
    void Visit(UpdateUndoRecord &record) override;
    void Visit(InsertUndoRecord &record) override;
};
} // namespace mi::access::heap::undo
