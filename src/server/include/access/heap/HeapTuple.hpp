#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/TupleId.hpp"
#include "access/table/AttrNumber.hpp"
#include "executor/Datum.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"

namespace mi::access::heap {

class HeapTuple : public table::ITuple {
  private:
    // Descriptor for this tuple
    const table::TupleDescriptor *_descriptor;
    // Tuple parsed from page
    HeapPageTuple _tuple;
    // Identifier of tuple
    TupleId _tid;

  public:
    HeapTuple(const table::TupleDescriptor *descriptor, HeapPageTuple &&tuple, TupleId tid);
    ~HeapTuple() = default;

    std::optional<Datum> GetAttribute(table::AttrNumber attrNumber) override;

    TupleId GetTID() const { return this->_tid; }

    HeapPageTupleHeader GetHeader() const { return this->_tuple.Header(); };
    const HeapPageTuple &GetHeapPageTuple() const { return this->_tuple; }
};
}; // namespace mi::access::heap
