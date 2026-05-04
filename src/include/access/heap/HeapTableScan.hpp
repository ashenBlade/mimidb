#pragma once

#include "access/heap/HeapTable.hpp"
#include "access/heap/TupleId.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "storage/BufferPin.hpp"
#include "storage/PageNumber.hpp"

namespace mi::access::heap {

class HeapTableScan : public table::ITableScan {
  private:
    // Id of page item id we are currently observing
    TupleId _tupleId;
    // Number of last page we want to observe (including).
    // Other pages will definitely be created by other transactions so will not be visible to us.
    storage::PageNumber _lastPageNumber;
    // Snapshot to check tuple visibility
    transam::Snapshot *_snapshot;
    // Table we are scanning
    access::heap::HeapTable *_table;
    // Scan is ended
    bool _end;

  public:
    HeapTableScan(transam::Snapshot *snapshot, access::heap::HeapTable *table);
    /// @brief Start iteration and prepare state
    void BeginScan() override;
    /// @brief Get next tuple from underlying table
    /// @return Tuple or NULL if end of scan
    std::unique_ptr<access::table::ITuple> GetNextTuple() override;
    /// @brief End iteration and cleanup resources
    void EndScan() override;
};

}; // namespace mi::access::heap
