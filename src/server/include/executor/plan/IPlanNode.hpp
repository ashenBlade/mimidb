#pragma once

#include "access/table/ITuple.hpp"
#include <memory>

namespace mi::executor::plan {

class IPlanNode {
  public:
    virtual void Start() = 0;
    virtual void End() = 0;
    virtual std::unique_ptr<mi::access::table::ITuple> Execute() = 0;

    virtual ~IPlanNode() = 0;
};

}; // namespace mi::executor::plan