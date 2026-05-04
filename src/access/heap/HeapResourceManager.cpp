#include "access/heap/undo/UndoApplierVisitor.hpp"
#include "mimidb.hpp"
#include "access/heap/undo/HeapUndoRecord.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/LogSeqNumber.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/UndoSeqNumber.hpp"
#include <stdexcept>

#include "access/heap/HeapResourceManager.hpp"

using namespace mi::access::heap;

void HeapResourceManager::ApplyUndo(mi::transam::IUndoRecord &record, transam::UndoSeqNumber usn) {
    assert(record.GetRMgrId() == transam::ResourceManagerId::Heap);

    auto &rec = dynamic_cast<undo::HeapUndoRecord &>(record);
    auto visitor = undo::UndoApplierVisitor{usn};
    rec.Accept(visitor);
}

void HeapResourceManager::ApplyRedo([[maybe_unused]] mi::transam::IWalRecord &record, [[maybe_unused]] transam::LogSeqNumber lsn) {
    throw std::runtime_error("not implemented");
}

HeapResourceManager *HeapResourceManager::Create() {
    return new HeapResourceManager();
}
