#include "access/table/ITableScan.hpp"
#include "mimidb.hpp"

#include "access/heap/HeapTable.hpp"
#include "access/heap/HeapTableScan.hpp"
#include "storage/RelFile.hpp"

#include <fcntl.h>
#include <stdexcept>

using namespace mi::access::heap;
using ITuple = mi::access::table::ITuple;

HeapTable::HeapTable(Oid tableId, const TupleDescriptor *descriptor)
    : _tableId(tableId), _tupleDescriptor(descriptor) {}

void HeapTable::InsertTuple([[maybe_unused]] std::shared_ptr<ITuple> tuple) {
    throw std::runtime_error("not implemented");
}

void HeapTable::UpdateTuple([[maybe_unused]] std::shared_ptr<ITuple> oldTuple,
                            [[maybe_unused]] std::shared_ptr<ITuple> newTuple) {
    throw std::runtime_error("not implemented");
}

std::unique_ptr<mi::access::table::ITableScan>
HeapTable::StartScan(std::shared_ptr<mi::transam::Snapshot> snapshot) {
    return std::make_unique<HeapTableScan>(snapshot, this);
}

void HeapTable::DeleteTuple([[maybe_unused]] std::shared_ptr<ITuple> tuple) {
    throw std::runtime_error("not implemented");
}

// Return number of pages for given relation. If
mi::storage::PageNumber HeapTable::GetPageCount() {
    auto file = storage::RelFile::Open(this->_tableId, O_RDONLY);
    return file.GetPagesCount();
}
