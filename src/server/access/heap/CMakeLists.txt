target_sources(
    ${PROJECT_NAME} PRIVATE
    HeapPage.cpp
    HeapTable.cpp
    HeapTableScan.cpp
    HeapTuple.cpp
    HeapTupleSerializer.cpp
    HeapResourceManager.cpp

    undo/UndoApplierVisitor.cpp
    undo/DeleteUndoRecord.cpp
    undo/UpdateUndoRecord.cpp
)