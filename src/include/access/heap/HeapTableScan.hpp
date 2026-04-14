#pragma once

#include "access/heap/HeapTable.hpp"
#include "access/heap/TupleId.hpp"
#include "access/table/ITableScan.hpp"
#include "access/table/ITuple.hpp"
#include "storage/PageNumber.hpp"
#include "storage/File.hpp"
#include "storage/BufferWrapper.hpp"

namespace mi::access::heap {

class HeapTableScan : public mi::access::table::ITableScan {
  private:
    // Id of page item id we are currently observing
    TupleId _tupleId;
    // File descriptor used to read table contents
    mi::storage::File _file;
    // Number of last page we want to observe (including).
    // Other pages will definitely be created by other transactions so will not be visible to us.
    mi::storage::PageNumber _lastPageNumber;
    // Snapshot to check tuple visibility
    std::shared_ptr<mi::transam::Snapshot> _snapshot;
    // Table we are scanning
    mi::access::heap::HeapTable &_table;
    // Buffer we have read and pinned
    mi::storage::BufferPin _buffer;

  public:
    HeapTableScan(std::shared_ptr<mi::transam::Snapshot> snapshot,
                  mi::access::heap::HeapTable &table);
    /// @brief Start iteration and prepare state
    void BeginScan() override;
    /// @brief Get next tuple from underlying table
    /// @return Tuple or NULL if end of scan
    std::unique_ptr<mi::access::table::ITuple> GetNextTuple() override;
    /// @brief End iteration and cleanup resources
    void EndScan() override;
};

}; // namespace mi::access::heap
