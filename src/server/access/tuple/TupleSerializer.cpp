#include "TupleSerializer.hpp"
#include "executor/Datum.hpp"
#include "utils/BitUtils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace mi::access::tuple;

TupleSerializer::TupleSerializer(const TupleDescriptor *desc) : _desc(desc) {
    assert(desc != nullptr);
}

SerializedTupleInfo TupleSerializer::Serialize(const std::vector<Datum> &values,
                                               const std::vector<bool> &isnull) {
    auto &attrs = *this->_desc->Attributes();
    auto maxAttno = this->_desc->GetMaxAttrNumber();
    auto hasnulls = false;

    // Calculate result tuple size and allocate buffer for it
    auto tupsize = 0UL;

    // First, actual values sizes
    for (auto attno = AttrNumber::Min(); attno <= maxAttno; ++attno) {
        auto idx = attno.ToIndex();
        if (isnull[idx]) {
            hasnulls = true;
            continue;
        }

        auto &attrdesc = attrs[idx];
        if (attrdesc.ByVal()) {
            // By val values' length is fixed in size
            assert(attrdesc.Length() > 0);
            tupsize += static_cast<size_t>(attrdesc.Length());
        } else if (attrdesc.Length() > 0) {
            // By ref with fixed size
            tupsize += static_cast<size_t>(attrdesc.Length());
        } else {
            // By ref with variable size
            auto length = *(values[idx].getPointer<uint32_t>());
            assert(length > 0);

            // Length field
            tupsize += sizeof(uint32_t);
            // Data itself
            tupsize += static_cast<size_t>(length);
        }
    }

    // Then add size of NULL bitmap
    if (hasnulls) {
        tupsize += BitUtils::BitmapSize(maxAttno.value);
    }

    // Allocate buffer for result tuple and perform serialization
    auto buffer = std::vector<std::byte>{tupsize};
    auto cursor = buffer.data();

    // First goes NULL bitmap
    if (hasnulls) {
        auto bitmap = reinterpret_cast<uint8_t *>(cursor);
        auto bmsize = BitUtils::BitmapSize(maxAttno.value);

        // Initialize and mark each attribute NULL
        memset(bitmap, 0, bmsize);

        // highbit is a highest bit that can be set in single byte.
        constexpr auto highbit = 0x80;
        // marks which bit in current byte we are going to set.
        // at start of each iteration it points to *previous* bitmask, so when
        // it equals to highbit then we initialize new chunk (go to next byte
        // and reset bitmask to 0x01)
        auto bitmask = highbit;

        // Hack to handle initial iteration - we see that bitmask is 'highbit'
        // and then go to the next byte which will be initial
        bitmap--;

        for (auto attno = AttrNumber::Min(); attno <= maxAttno; ++attno) {
            auto idx = attno.ToIndex();
            if (bitmask == highbit) {
                // We are starting new byte
                bitmask = 1;
                bitmap++;
            } else {
                // Shift to new attribute/bit
                bitmask <<= 1;
            }

            if (!isnull[idx]) {
                // Mark attribute as non-NULL
                *bitmap |= static_cast<uint8_t>(bitmask);
            }
        }

        // advance cursor with bitmap size
        cursor += bmsize;
    }

    // Finally actual data
    for (auto attno = AttrNumber::Min(); attno <= maxAttno; ++attno) {
        // NULLs are not written
        auto idx = attno.ToIndex();
        if (isnull[idx]) {
            continue;
        }

        auto value = values[idx];
        auto &attr = attrs[idx];
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
                throw std::runtime_error("invalid by val attr length");
            }
        } else if (attr.Length() > 0) {
            // Fixed-sized by ref
            auto data = value.getPointer<std::byte>();
            std::memcpy(cursor, data, static_cast<size_t>(attr.Length()));

            cursor += static_cast<size_t>(attr.Length());
        } else {
            // Variable-sized by ref
            auto data = value.getPointer<std::byte>();

            // Write out length
            auto length = *reinterpret_cast<int32_t *>(data);
            *reinterpret_cast<int32_t *>(cursor) = length;
            cursor += sizeof(int32_t);

            // Now data itself
            data += sizeof(int32_t);
            std::memcpy(cursor, data, static_cast<size_t>(length));

            cursor += static_cast<uint32_t>(length);
        }
    }

    return SerializedTupleInfo{std::move(buffer), hasnulls};
}

// Parse current attribute and returns pair of stored attribute's value and it's full length
static std::pair<mi::Datum, size_t>
extract_attr_datum(const std::byte *cursor, const mi::access::AttributeDescriptor &desc) {
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
    } else if (desc.Length() > 0) {
        value = mi::Datum{cursor};
        length = static_cast<size_t>(desc.Length());
    } else {
        value = mi::Datum{cursor};

        // First field contains length of remaining data
        auto dataLen = *reinterpret_cast<const int32_t *>(cursor);
        length = sizeof(int32_t) + static_cast<size_t>(dataLen);
    }
    return std::make_pair(value, length);
}

DeserializedTuple TupleSerializer::Deserialize(const std::byte *array, bool hasnulls,
                                               [[maybe_unused]] size_t size) {

    auto maxattno = this->_desc->GetMaxAttrNumber();
    auto values = std::vector<mi::Datum>(maxattno);
    auto isnull = std::vector<bool>(maxattno);

    auto cursor = array;
    if (hasnulls) {
        auto bitmap = reinterpret_cast<const uint8_t *>(cursor);
        for (auto attno = AttrNumber::Min(); attno <= maxattno; attno++) {
            auto idx = attno.ToIndex();
            isnull[idx] = !(bitmap[idx >> 3] & (1 << (idx & 0x07)));
        }

        cursor += BitUtils::BitmapSize(maxattno.value);
    } else {
        std::fill(isnull.begin(), isnull.end(), false);
    }

    // Now parse actual data
    auto &attributes = *this->_desc->Attributes();
    for (auto attno = AttrNumber::Min(); attno <= maxattno; attno++) {
        auto num = attno.ToIndex();
        if (hasnulls && isnull[num]) {
            continue;
        }

        auto &desc = attributes[num];
        auto [value, length] = extract_attr_datum(cursor, desc);
        values[num] = value;
        cursor += length;
    }

    return DeserializedTuple{std::move(values), std::move(isnull)};
}
