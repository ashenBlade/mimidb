#pragma once

#include <memory>

#include "access/table/ITableScan.hpp"
#include "executor/ITuple.hpp"
#include "transam/Snapshot.hpp"
#include "schema/TupleDescriptor.hpp"

namespace mi::access::table {

class ITable {
  public:
    using ITuple = mi::executor::ITuple;

    virtual std::shared_ptr<mi::schema::TupleDescriptor> GetDescriptor();

    /// @brief Create scan state for given table
    /// @param snapshot Snapshot to check tuple visibility
    /// @return Scan state which performs actual scan
    virtual std::unique_ptr<ITableScan> StartScan(std::shared_ptr<mi::transam::Snapshot> snapshot) = 0;
    /// @brief Insert new tuple into table
    /// @param tuple Tuple to insert
    virtual void InsertTuple(std::shared_ptr<ITuple> tuple) = 0;
    /// @brief Update existing tuple with new one 
    /// @param oldTuple Tuple to update
    /// @param newTuple New tuple contents
    virtual void UpdateTuple(std::shared_ptr<ITuple> oldTuple,
                             std::shared_ptr<ITuple> newTuple) = 0;
    /// @brief Delete existing tuple
    /// @param tuple Tuple to delete
    virtual void DeleteTuple(std::shared_ptr<ITuple> tuple) = 0;
};

}; // namespace mi::access::table
