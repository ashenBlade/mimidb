#pragma once

#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include "access/table/ITuple.hpp"
#include "access/table/TupleDescriptor.hpp"

#include <memory>

namespace mi::access::heap {

class HeapTuple : public table::ITuple {
  private:
    // Pointer to actual tuple data
    std::unique_ptr<HeapPageTupleHeader> _tuple;
    // Descriptor for this tuple
    std::shared_ptr<table::TupleDescriptor> _descriptor;
    // Length of tuple
    int16_t _length;
    // Flag indicating whether tuple is processed or not.
    // true means _values/_isnull are valid
    bool _processed;
    // Runtime parsed values from tuple
    std::vector<table::Datum> _values;
    // NULL flags for each attribute
    std::vector<bool> _isnull;

    void parseTuple();
  public:
    HeapTuple(std::shared_ptr<table::TupleDescriptor> descriptor,
              std::unique_ptr<HeapPageTupleHeader> tuple, uint32_t length);

    std::optional<table::Datum> GetAttribute(table::AttrNumber attrNumber) override;
};
}; // namespace mi::access::heap
