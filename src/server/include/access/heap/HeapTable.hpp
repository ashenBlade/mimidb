#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/table/ITable.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/Oid.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageNumber.hpp"

#include "utils/NonCopyable.hpp"

namespace mi::access::heap {

/// @brief ITable implementation for heap table
class HeapTable : public mi::access::table::ITable, private NonCopyable {
  private:
    using TupleDescriptor = mi::access::table::TupleDescriptor;
    using ITuple = mi::access::table::ITuple;

    // Object id of given table
    Oid _tableId;

    // Table schema descriptor
    const TupleDescriptor *_tupleDescriptor;

    // Amount of pages in given relation so far. If Invalid, then unknown
    mi::storage::PageNumber _pageCountCached;

    HeapPageTuple formHeapPageTuple(ITuple &tuple) const;
    storage::BufferPin searchPageFreeSpace(size_t freeSpace) const;

  public:
    HeapTable(Oid tableId, const TupleDescriptor *descriptor);
    ~HeapTable() = default;

    Oid GetOid() const { return this->_tableId; }
    const TupleDescriptor *GetDescriptor() const override { return this->_tupleDescriptor; }

    std::unique_ptr<mi::access::table::ITableScan>
    StartScan(mi::transam::Snapshot *snapshot) override;
    void InsertTuple(ITuple &tuple) override;
    void UpdateTuple(ITuple &oldTuple, ITuple &newTuple) override;
    void DeleteTuple(ITuple &tuple) override;

    // Return number of pages for given relation. If
    mi::storage::PageNumber GetPageCount();
};

}; // namespace mi::access::heap
