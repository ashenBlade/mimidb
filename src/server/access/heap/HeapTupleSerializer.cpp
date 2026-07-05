#include "access/heap/HeapTupleSerializer.hpp"
#include "access/TupleDescriptor.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/tuple/TupleSerializer.hpp"
#include <cstddef>
#include <cstring>
#include <vector>

using namespace mi::access::heap;

std::vector<std::byte> HeapTupleSerializer::Serialize(const HeapPageTuple &tuple, const TupleDescriptor &desc) {
    auto serializer = tuple::TupleSerializer{&desc};
    auto sti = serializer.Serialize(tuple.Values(), tuple.Nulls());
    auto buffer = std::vector<std::byte>(sti.Tuple.size() + sizeof(HeapPageTupleHeader));

    auto cursor = buffer.data();
    auto header = reinterpret_cast<HeapPageTupleHeader *>(cursor);
    *header = tuple.Header();

    // Update flags
    if (sti.HasNulls) {
        header->flags = HeapTupleFlags::HasNulls;
    } else {
        header->flags = static_cast<HeapTupleFlags>(0);
    }

    cursor += sizeof(HeapPageTupleHeader);
    std::memcpy(cursor, sti.Tuple.data(), sti.Tuple.size());

    return buffer;
}

HeapPageTuple HeapTupleSerializer::Deserialize(const std::byte *array, size_t size,
                                               const TupleDescriptor &desc) {
    auto header = reinterpret_cast<const HeapPageTupleHeader *>(array);
    auto serializer = tuple::TupleSerializer{&desc};
    auto payload = array + sizeof(HeapPageTupleHeader);
    auto deserialized = serializer.Deserialize(payload, header->flags & HeapTupleFlags::HasNulls,
                                               size - sizeof(HeapPageTupleHeader));

    return HeapPageTuple{*header, std::move(deserialized.Values), std::move(deserialized.IsNull)};
}
