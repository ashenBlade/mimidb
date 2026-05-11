#pragma once

#include "access/heap/undo/InsertUndoRecord.hpp"
#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/heap/undo/UpdateUndoRecord.hpp"

namespace mi::access::heap::undo {
class IHeapUndoRecordVisitor {
  public:
    virtual void Visit(DeleteUndoRecord &record) = 0;
    virtual void Visit(UpdateUndoRecord &record) = 0;
    virtual void Visit(InsertUndoRecord &record) = 0;
    virtual ~IHeapUndoRecordVisitor() = default;
};
} // namespace mi::access::heap::undo
