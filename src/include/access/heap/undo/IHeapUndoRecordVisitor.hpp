#pragma once

#include "access/heap/undo/DeleteUndoRecord.hpp"

namespace mi::access::heap::undo {
class IHeapUndoRecordVisitor {
  public:
    virtual void Visit(DeleteUndoRecord &record) = 0;
    virtual ~IHeapUndoRecordVisitor() = default;
};
} // namespace mi::access::heap::undo
