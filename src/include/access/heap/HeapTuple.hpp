#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"

#include <memory>

namespace mi::access::heap {

class HeapTuple : public table::ITuple {
  private:
    // Descriptor for this tuple
    const table::TupleDescriptor *_descriptor;
    // Tuple parsed from page
    HeapPageTuple _tuple;

  public:
    HeapTuple(const table::TupleDescriptor *descriptor, HeapPageTuple &&tuple);
    ~HeapTuple() = default;

    std::optional<Datum> GetAttribute(table::AttrNumber attrNumber) override;
};
}; // namespace mi::access::heap
