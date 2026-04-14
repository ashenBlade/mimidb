#pragma once

#include "access/table/ITable.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/Oid.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "storage/PageNumber.hpp"
#include "storage/File.hpp"

#include "utils/NonCopyable.hpp"

namespace mi::access::heap {

/// @brief ITable implementation for heap table
class HeapTable : public mi::access::table::ITable, private NonCopyable {
  private:
    using Oid = mi::access::table::Oid;
    using TupleDescriptor = mi::access::table::TupleDescriptor;
    using ITuple = mi::access::table::ITuple;

    // Object id of given table
    Oid _tableId;

    // Table schema descriptor
    std::shared_ptr<TupleDescriptor> _tupleDescriptor;
    
    // Amount of pages in given relation so far. If Invalid, then unknown
    mi::storage::PageNumber _pageCountCached;

  public:
    HeapTable(Oid tableId, std::shared_ptr<TupleDescriptor> descriptor);

    Oid GetOid() const;
    TupleDescriptor &GetDescriptor() override;

    std::unique_ptr<mi::access::table::ITableScan> StartScan(std::shared_ptr<mi::transam::Snapshot> snapshot) override;
    void InsertTuple(std::shared_ptr<ITuple> tuple) override;
    void UpdateTuple(std::shared_ptr<ITuple> oldTuple, std::shared_ptr<ITuple> newTuple) override;
    void DeleteTuple(std::shared_ptr<ITuple> tuple) override;

    // Return number of pages for given relation. If
    mi::storage::PageNumber GetPageCount();
    // Open underlying file storage
    mi::storage::File Open();
};

}; // namespace mi::access::heap
