#pragma once

#include "executor/ITuple.hpp"

namespace mi::access::heap {

class HeapTuple : public mi::executor::ITuple {
    mi::executor::Datum GetAttribute(mi::schema::AttrNumber attrNumber);
};

};
