#pragma once

#include "access/heap/HeapTable.hpp"
#include "access/heap/TupleId.hpp"
#include "access/table/ITableScan.hpp"
#include "storage/BufferManager.hpp"
#include "storage/PageNumber.hpp"

namespace mi::access::heap {

class HeapTableScan : public mi::access::table::ITableScan {
  private:
    /// @brief Number of page we are currently observing
    mi::storage::PageNumber _pageNumber;
    /// @brief Id of page item id we are currently observing
    TupleId _tupleId;
    /// @brief Buffer manager to access pages
    std::shared_ptr<mi::storage::BufferManager> _bufferManager;
    /// @brief Snapshot to check tuple visibility
    mi::transam::Snapshot _snapshot;

  public:
    /// @brief Start iteration and prepare state
    void BeginScan() override;
    /// @brief Get next tuple from underlying table
    /// @return Tuple or NULL if end of scan
    std::unique_ptr<mi::executor::ITuple> GetNextTuple() override;
    /// @brief End iteration and cleanup resources
    void EndScan() override;
};

}; // namespace mi::access::heap
