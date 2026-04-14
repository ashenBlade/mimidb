#include "mimidb.hpp"

#include "access/heap/HeapTable.hpp"
#include "access/heap/HeapTableScan.hpp"

#include <stdexcept>

using namespace mi::access::heap;
using ITuple = mi::access::table::ITuple;

void HeapTable::InsertTuple(std::shared_ptr<ITuple> tuple) {}

HeapTable::HeapTable(Oid tableId, std::shared_ptr<mi::schema::TupleDescriptor> descriptor)
    : _tableId(tableId), _tupleDescriptor(descriptor) {}

void HeapTable::UpdateTuple(std::shared_ptr<ITuple> oldTuple, std::shared_ptr<ITuple> newTuple) {
    throw std::runtime_error("not implemented");
}

std::unique_ptr<ITableScan> HeapTable::StartScan(std::shared_ptr<mi::storage::BufferManager> bufferManager,
                                                 std::shared_ptr<mi::transam::Snapshot> snapshot) {
    return std::make_unique<HeapTableScan>(bufferManager, snapshot, *this);
}

void HeapTable::DeleteTuple(std::shared_ptr<ITuple> tuple) { throw std::runtime_error("not implemented"); }
