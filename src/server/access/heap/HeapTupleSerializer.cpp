#include "access/heap/HeapTupleSerializer.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/table/AttrNumber.hpp"
#include "access/table/TupleDescriptor.hpp"
#include "mi_config.hpp"
#include "utils/BitUtils.hpp"
#include <bitset>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace mi::access::heap;

uint16_t HeapTupleSerializer::CalculateSize(const HeapPageTuple &tuple,
                                            const table::TupleDescriptor &desc) {
    auto totalSize = static_cast<uint16_t>(tuple.Header().dataStartOffset);
    auto maxAttno = desc.GetMaxAttrNumber();
    auto &isnull = tuple.Nulls();
    auto &values = tuple.Values();
    auto &attrs = desc.Attributes();
    for (auto attno = table::AttrNumber::Min(); attno <= maxAttno; attno++) {
        if (isnull[attno.ToIndex()]) {
            // NULLs are not written
            continue;
        }

        auto &attr = attrs[attno.ToIndex()];
        if (attr.ByVal()) {
            // Scalars have fixed length
            assert(attr.Length() > 0);
            totalSize += static_cast<uint16_t>(attr.Length());
        } else {
            // For reference types we must get length by dereferencing
            auto length = *values[attno.ToIndex()].getPointer<int32_t>();
            assert(length > 0);
            // Length field
            totalSize += sizeof(int32_t);
            // Data itself
            totalSize += BitUtils::MaxAlign(static_cast<uint16_t>(length));
        }
    }

    return totalSize;
}

std::vector<std::byte> HeapTupleSerializer::Serialize(const HeapPageTuple &tuple,
                                                      const table::TupleDescriptor &desc,
                                                      size_t size) {
    auto isnull = tuple.Nulls();
    auto values = tuple.Values();
    auto maxAttno = desc.GetMaxAttrNumber();
    auto attrs = desc.Attributes();

    // Now actually serialize tuple
    auto buffer = std::vector<std::byte>(size);
    auto cursor = buffer.data();

    // Just copy header as is
    *reinterpret_cast<HeapPageTupleHeader *>(cursor) = tuple.Header();
    cursor += sizeof(HeapPageTupleHeader);

    // Add NULL bitmap right after it
    if (tuple.Header().flags & HeapTupleFlags::HasNulls) {
        uint8_t *bitmap = reinterpret_cast<uint8_t *>(cursor);

        auto nfullbytes = isnull.size() / Config::BitsPerByte;
        auto nbytes = BitUtils::BitmapSize(isnull.size());
        auto byte = 0U;
        for (; byte < nbytes; byte++) {
            auto set = std::bitset<Config::BitsPerByte>{0};
            auto maxBits =
                byte == nfullbytes ? isnull.size() % Config::BitsPerByte : Config::BitsPerByte;

            for (auto bit = 0U; bit < maxBits; ++bit) {
                set.set(bit, !isnull[byte * 8 + bit]);
            }

            auto val = set.to_ulong();
            assert(static_cast<uint8_t>(val) == val);
            bitmap[byte] = static_cast<uint8_t>(val);

            // Go to next byte
            bitmap++;
        }
    }

    // Serialize data itself
    cursor = buffer.data() + tuple.Header().dataStartOffset;
    for (auto attno = table::AttrNumber::Min(); attno <= maxAttno; ++attno) {
        // NULL is not written
        if (isnull[attno.ToIndex()]) {
            continue;
        }

        auto value = values[attno.ToIndex()];
        auto &attr = attrs[attno.ToIndex()];
        if (attr.ByVal()) {
            switch (attr.Length()) {
            case 1:
                *reinterpret_cast<int8_t *>(cursor) = value.getScalar<int8_t>();
                cursor += sizeof(int8_t);
                break;
            case 2:
                *reinterpret_cast<int16_t *>(cursor) = value.getScalar<int16_t>();
                cursor += sizeof(int16_t);
                break;
            case 4:
                *reinterpret_cast<int32_t *>(cursor) = value.getScalar<int32_t>();
                cursor += sizeof(int32_t);
                break;
            case 8:
                *reinterpret_cast<int64_t *>(cursor) = value.getScalar<int64_t>();
                cursor += sizeof(int64_t);
                break;
            default:
                throw std::runtime_error("invalid attr by val length");
            }
        } else {
            auto data = value.getPointer<std::byte>();

            // Write out length
            auto length = *reinterpret_cast<int32_t *>(data);
            *reinterpret_cast<int32_t *>(cursor) = length;
            cursor += sizeof(int32_t);

            // Now data itself
            data += sizeof(int32_t);
            std::memcpy(cursor, data, static_cast<size_t>(length));

            cursor += BitUtils::MaxAlign(static_cast<uint32_t>(length));
        }
    }

    return buffer;
}

// Parse current attribute and returns pair of stored attribute's value and it's full length
static std::pair<mi::Datum, size_t>
extract_attr_datum(const std::byte *cursor, const mi::access::table::AttributeDescriptor &desc) {
    mi::Datum value;
    size_t length;

    if (desc.ByVal()) {
        // By-value type - read length exactly
        length = static_cast<size_t>(desc.Length());
        assert(length > 0);
        switch (length) {
        case sizeof(int8_t):
            value = mi::Datum{*reinterpret_cast<const int8_t *>(cursor)};
            break;
        case sizeof(int16_t):
            value = mi::Datum{*reinterpret_cast<const int16_t *>(cursor)};
            break;
        case sizeof(int32_t):
            value = mi::Datum{*reinterpret_cast<const int32_t *>(cursor)};
            break;
        case sizeof(int64_t):
            value = mi::Datum{*reinterpret_cast<const int64_t *>(cursor)};
            break;
        default:
            assert(false);
            throw std::runtime_error("unknown type length " + std::to_string(length));
        }
    } else {
        value = mi::Datum{cursor};

        // First field contains length of remaining data
        auto dataLen = *reinterpret_cast<const int32_t *>(cursor);
        length = sizeof(int32_t) + static_cast<size_t>(dataLen);
    }
    return std::make_pair(value, length);
}

HeapPageTuple HeapTupleSerializer::Deserialize(const std::byte *array,
                                               const table::TupleDescriptor &desc) {
    auto natts = desc.GetMaxAttrNumber();
    auto values = std::vector<mi::Datum>(natts);
    auto isnull = std::vector<bool>(natts);

    auto header = reinterpret_cast<const HeapPageTupleHeader *>(array);
    auto hasnulls = header->flags & HeapTupleFlags::HasNulls;
    if (hasnulls) {
        auto bitmap = reinterpret_cast<const uint8_t *>(array + sizeof(HeapPageTupleHeader));
        for (auto attno = table::AttrNumber::Min(); attno <= natts; attno++) {
            auto num = attno.ToIndex();
            isnull[num] = !(bitmap[num >> 3] & (1 << (num & 0x07)));
        }
    } else {
        std::fill(isnull.begin(), isnull.end(), false);
    }

    // Now parse actual data
    auto tupdata = array + header->dataStartOffset;
    auto attributes = desc.Attributes();
    for (auto attno = table::AttrNumber::Min(); attno <= natts; attno++) {
        auto num = attno.ToIndex();
        if (hasnulls && isnull[num]) {
            continue;
        }

        auto &desc = attributes[num];
        auto [value, length] = extract_attr_datum(tupdata, desc);
        values[num] = value;
        tupdata += length;
    }

    return HeapPageTuple{*header, std::move(values), std::move(isnull)};
}
