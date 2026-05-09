#include "access/heap/undo/DeleteUndoRecord.hpp"
#include "access/table/Oid.hpp"
#include "mimidb.hpp"

#include "access/heap/undo/HeapUndoRecord.hpp"
#include "access/heap/undo/UndoApplierVisitor.hpp"
#include "transam/IUndoRecord.hpp"
#include "transam/IWalRecord.hpp"
#include "transam/LogSeqNumber.hpp"
#include "transam/ResourceManagerId.hpp"
#include "transam/UndoSeqNumber.hpp"
#include <memory>
#include <stdexcept>
#include <string>

#include "access/heap/HeapResourceManager.hpp"

using namespace mi::access::heap;

void HeapResourceManager::ApplyUndo(mi::transam::IUndoRecord &record, transam::UndoSeqNumber usn) {
    assert(record.GetRMgrId() == transam::ResourceManagerId::Heap);

    auto &rec = dynamic_cast<undo::HeapUndoRecord &>(record);
    auto visitor = undo::UndoApplierVisitor{usn};
    rec.Accept(visitor);
}

void HeapResourceManager::ApplyRedo([[maybe_unused]] mi::transam::IWalRecord &record,
                                    [[maybe_unused]] transam::LogSeqNumber lsn) {
    throw std::runtime_error("not implemented");
}

HeapResourceManager *HeapResourceManager::Create() { return new HeapResourceManager(); }

std::unique_ptr<mi::transam::IUndoRecord> HeapResourceManager::ParseUndo(uint8_t t, std::byte *data, size_t length) {
    auto type = static_cast<undo::HeapUndoRecordType>(t);
    switch (type) {
        case mi::access::heap::undo::HeapUndoRecordType::Delete: {
            if (length != undo::DeleteUndoRecord::Size) {
                throw std::runtime_error("record size does not match");
            }

            auto cursor = data;

            auto tableId = *reinterpret_cast<Oid *>(cursor);
            cursor += sizeof(Oid);
            auto tupleId = *reinterpret_cast<TupleId *>(cursor);

            return std::make_unique<undo::DeleteUndoRecord>(tableId, tupleId);
        }
    }

    throw std::runtime_error("unknown heap record type: " + std::to_string(t));
}
