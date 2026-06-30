#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/TupleId.hpp"
#include "access/AttrNumber.hpp"
#include "executor/Datum.hpp"
#include "access/ITuple.hpp"
#include "access/TupleDescriptor.hpp"

namespace mi::access::heap {

class HeapTuple : public ITuple {
  private:
    // Descriptor for this tuple
    const TupleDescriptor *_descriptor;
    // Tuple parsed from page
    HeapPageTuple _tuple;
    // Identifier of tuple
    TupleId _tid;

  public:
    HeapTuple(const TupleDescriptor *descriptor, HeapPageTuple &&tuple, TupleId tid);
    ~HeapTuple() = default;

    std::optional<Datum> GetAttribute(AttrNumber attrNumber) override;
    AttrNumber GetMaxAttno() override;

    TupleId GetTID() const { return this->_tid; }

    HeapPageTupleHeader GetHeader() const { return this->_tuple.Header(); };
    const HeapPageTuple &GetHeapPageTuple() const { return this->_tuple; }
};
}; // namespace mi::access::heap
