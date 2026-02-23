#pragma once

#include "access/table/ITable.hpp"

namespace mi::access::heap {

/// @brief ITable implementation for heap table
class HeapTable : public mi::access::table::ITable {
  private:
    /// @brief Table schema descriptor
    std::shared_ptr<mi::schema::TupleDescriptor> _tupleDescriptor;

  public:
    std::unique_ptr<mi::access::table::ITableScan> StartScan(std::shared_ptr<mi::transam::Snapshot> snapshot);
    void InsertTuple(std::shared_ptr<ITuple> tuple);
    void UpdateTuple(std::shared_ptr<ITuple> oldTuple, std::shared_ptr<ITuple> newTuple);
    void DeleteTuple(std::shared_ptr<ITuple> tuple);
};

}; // namespace mi::access::heap
