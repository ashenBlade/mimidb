#pragma once

#include "access/ITableScan.hpp"
#include "access/ITuple.hpp"
#include "access/TupleDescriptor.hpp"
#include "trans/Snapshot.hpp"

namespace mi::access {

class ITable {
  public:
    virtual const TupleDescriptor *GetDescriptor() const = 0;

    /// @brief Create scan state for given table
    /// @param snapshot Snapshot to check tuple visibility
    /// @return Scan state which performs actual scan
    virtual std::unique_ptr<ITableScan> StartScan(mi::storage::trans::Snapshot *snapshot) = 0;
    /// @brief Insert new tuple into table
    /// @param tuple Tuple to insert
    virtual void InsertTuple(ITuple &tuple) = 0;
    /// @brief Update existing tuple with new one
    /// @param oldTuple Tuple to update. Type must be exactly the same as one's that returned by
    /// scan
    /// @param newTuple New tuple contents
    virtual void UpdateTuple(ITuple &oldTuple, ITuple &newTuple) = 0;
    /// @brief Delete existing tuple
    /// @param tuple Tuple to delete
    virtual void DeleteTuple(ITuple &tuple) = 0;

    virtual ~ITable() = default;
};

}; // namespace mi::access
