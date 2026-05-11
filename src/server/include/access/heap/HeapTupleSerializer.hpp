#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/table/TupleDescriptor.hpp"

namespace mi::access::heap {

class HeapTupleSerializer {
  public:
    // Calculate size of tuple only. No trailing padding added.
    static uint16_t CalculateSize(const HeapPageTuple &tuple, const table::TupleDescriptor &desc);
    // Serialize tuple
    static std::vector<std::byte> Serialize(const HeapPageTuple &tuple,
                                            const table::TupleDescriptor &desc, size_t size);
    static HeapPageTuple Deserialize(const std::byte *array, const table::TupleDescriptor &desc);
};

} // namespace mi::access::heap
