#pragma once

#include "access/heap/HeapPageTuple.hpp"
#include "access/TupleDescriptor.hpp"

namespace mi::access::heap {

class HeapTupleSerializer {
  public:
    // Calculate size of tuple only. No trailing padding added.
    static uint16_t CalculateSize(const HeapPageTuple &tuple, const TupleDescriptor &desc);
    // Serialize tuple
    static std::vector<std::byte> Serialize(const HeapPageTuple &tuple,
                                            const TupleDescriptor &desc, size_t size);
    static HeapPageTuple Deserialize(const std::byte *array, const TupleDescriptor &desc);
};

} // namespace mi::access::heap
