#pragma once

#include "access/TupleDescriptor.hpp"
#include "executor/Datum.hpp"
#include <cstddef>
#include <vector>

namespace mi::access::tuple {

struct SerializedTupleInfo {
    // Actual serialized tuple
    std::vector<std::byte> Tuple;
    // Flag indicating that this tuple has NULLs.
    // When deserializing you must pass this flag in order to properly deserialize.
    bool HasNulls;
};

struct DeserializedTuple {
    std::vector<Datum> Values;
    std::vector<bool> IsNull;
};

class TupleSerializer {
  private:
    // Descriptor for tuple
    const TupleDescriptor *_desc;

  public:
    TupleSerializer(const TupleDescriptor *desc);

    SerializedTupleInfo Serialize(const std::vector<Datum> &values, const std::vector<bool> &isnull);
    DeserializedTuple Deserialize(const std::byte *array, bool hasnulls, size_t size);
};
} // namespace mi::access::tuple
