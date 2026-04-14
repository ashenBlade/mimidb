#pragma once

#include "access/table/ITuple.hpp"

namespace mi::access::heap {

class HeapTuple : public mi::access::table::ITuple {
    std::optional<mi::access::table::Datum> GetAttribute(mi::access::table::AttrNumber attrNumber);
};

};
