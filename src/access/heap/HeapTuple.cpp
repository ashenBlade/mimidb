#include "mimidb.hpp"

#include "access/heap/HeapPageTupleHeader.hpp"

#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include <stdexcept>
#include <string>
#include <utility>

#include "access/heap/HeapTuple.hpp"

using namespace mi::access::heap;
using namespace mi::access;
using namespace mi::access::table;

std::optional<mi::Datum> HeapTuple::GetAttribute(table::AttrNumber attno) {
    if (this->_descriptor->GetMaxAttrNumber() < attno) {
        throw std::runtime_error("provided attribute number greater than available");
    }

    if (this->_tuple.Nulls()[attno.ToIndex()]) {
        return std::nullopt;
    } else {
        return this->_tuple.Values()[attno.ToIndex()];
    }
}

HeapTuple::HeapTuple(const table::TupleDescriptor *descriptor, HeapPageTuple &&tuple)
    : _descriptor(descriptor), _tuple(std::move(tuple)) { };
