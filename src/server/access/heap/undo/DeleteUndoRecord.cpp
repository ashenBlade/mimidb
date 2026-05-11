#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/heap/undo/IHeapUndoRecordVisitor.hpp"

using namespace mi::access::heap::undo;

void DeleteUndoRecord::Accept(IHeapUndoRecordVisitor &visitor) { visitor.Visit(*this); }
