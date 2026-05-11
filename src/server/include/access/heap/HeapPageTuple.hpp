#pragma once

#include <vector>

#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/table/Datum.hpp"

namespace mi::access::heap {

// Tuple on heap page wrapper
class HeapPageTuple {
  private:
    // Tuple header
    HeapPageTupleHeader _header;
    // Actual values
    std::vector<Datum> _values;
    // NULL bitmap for values
    std::vector<bool> _isnull;

  public:
    HeapPageTuple(HeapPageTupleHeader header, std::vector<Datum> &&values,
                  std::vector<bool> &&isnull)
        : _header(header), _values(std::move(values)), _isnull(std::move(isnull)) {};

    HeapPageTupleHeader &Header() { return this->_header; }
    const HeapPageTupleHeader &Header() const { return this->_header; };

    std::vector<Datum> &Values() { return this->_values; };
    const std::vector<Datum> &Values() const { return this->_values; };

    std::vector<bool> Nulls() { return this->_isnull; };
    const std::vector<bool> Nulls() const { return this->_isnull; };
};

} // namespace mi::access::heap
