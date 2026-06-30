#pragma once

#include "access/ITuple.hpp"
#include "trans/Snapshot.hpp"
#include <memory>

namespace mi::executor::plan {

class PlanNode {
  protected:
    int64_t _nrows;

  public:
    PlanNode(): _nrows(0) {};
    virtual void Start(storage::trans::Snapshot *snapshot) = 0;
    virtual void End() = 0;
    virtual std::unique_ptr<mi::access::ITuple> Execute() = 0;

    int64_t GetRowsProcessed() { return this->_nrows; }

    virtual ~PlanNode() = default;
};

}; // namespace mi::executor::plan