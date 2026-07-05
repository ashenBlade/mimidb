#pragma once

#include "access/TupleDescriptor.hpp"
#include "access/heap/HeapPageTuple.hpp"

namespace mi::access::heap {

class HeapTupleSerializer {
  public:
    // Serialize tuple
    static std::vector<std::byte> Serialize(const HeapPageTuple &tuple, const TupleDescriptor &desc);
    static HeapPageTuple Deserialize(const std::byte *array, size_t size,
                                     const TupleDescriptor &desc);
};

} // namespace mi::access::heap
