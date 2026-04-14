#include "access/heap/HeapPage.hpp"
#include "mimidb.hpp"

#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

#include "access/heap/HeapTableScan.hpp"
#include "storage/BufferWrapper.hpp"
#include "storage/PageNumber.hpp"

using namespace mi::access::heap;

HeapTableScan::HeapTableScan(std::shared_ptr<mi::transam::Snapshot> snapshot,
                             mi::access::heap::HeapTable &table)
    : _tupleId(), _file(), _lastPageNumber(), _snapshot(snapshot), _table(table), _buffer() {}

void HeapTableScan::BeginScan() {
    auto lastPage = this->_table.GetPageCount();
    assert(lastPage.IsValid());
    this->_lastPageNumber = lastPage;
    this->_file = this->_table.Open();
    // Start iterating from first page. Note that BufferWrapper is initialized with Invalid, so we
    // will read buffer at first scan call
    this->_tupleId = TupleId{storage::PageNumber::Min, 0};

    // NOTE: lock is not needed - only SeqScan, no DML
};

std::unique_ptr<mi::access::table::ITuple> mi::access::heap::HeapTableScan::GetNextTuple() {
    if (this->_tupleId.pageno != this->_buffer.GetPageNumber()) {
        // This was the last page to scan
        if (this->_tupleId.pageno == this->_lastPageNumber) {
            return nullptr;
        }

        // Acquire required page now
        this->_buffer = storage::BufferPin::GetBuffer(this->_tupleId.pageno);
    }

    // Now buffer is pinned and we can proceed
    this->_buffer.Lock(true);
    auto page = HeapPage{this->_buffer.GetContents()};
    for (auto index = this->_tupleId.itemid + 1; index < page.ItemsCount(); ++index) {
        auto itemid = page.GetItemId(index);
        if (itemid.isUnused()) {
            // No data, even history - proceed
            continue;
        }
        
        // TODO: продолжаю логику чтения SeqScan
    }
}

void mi::access::heap::HeapTableScan::EndScan() { throw std::runtime_error("not implemented"); }
