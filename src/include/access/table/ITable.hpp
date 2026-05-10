#pragma once

#include <memory>

#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "transam/Snapshot.hpp"

namespace mi::access::table {

class ITable {
  public:
    virtual const TupleDescriptor *GetDescriptor() const = 0;

    /// @brief Create scan state for given table
    /// @param snapshot Snapshot to check tuple visibility
    /// @return Scan state which performs actual scan
    virtual std::unique_ptr<ITableScan> StartScan(mi::transam::Snapshot *snapshot) = 0;
    /// @brief Insert new tuple into table
    /// @param tuple Tuple to insert
    virtual void InsertTuple(ITuple &tuple) = 0;
    /// @brief Update existing tuple with new one
    /// @param oldTuple Tuple to update. Type must be exactly the same as one's that returned by scan
    /// @param newTuple New tuple contents
    virtual void UpdateTuple(ITuple &oldTuple, ITuple &newTuple) = 0;
    /// @brief Delete existing tuple
    /// @param tuple Tuple to delete
    virtual void DeleteTuple(ITuple &tuple) = 0;

    virtual ~ITable() = default;
};

}; // namespace mi::access::table
