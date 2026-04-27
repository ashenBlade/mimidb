#include "access/heap/HeapPageTupleHeader.hpp"
#include "mimidb.hpp"

#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include <algorithm>
#include <bitset>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "access/heap/HeapTuple.hpp"

using namespace mi::access::heap;
using namespace mi::access;
using namespace mi::access::table;

// Parse current attribute and returns pair of stored attribute's value and it's full length
static std::pair<Datum, size_t> extract_attr_datum(char *cursor, const AttributeDescriptor &desc) {
    Datum value;
    size_t length;

    if (desc.ByVal()) {
        // By-value type - read length exactly
        length = static_cast<size_t>(desc.Length());
        assert(length > 0);
        switch (length) {
            case sizeof(int8_t):
                value = Datum{*reinterpret_cast<int8_t *>(cursor)};
                break;
            case sizeof(int16_t):
                value = Datum{*reinterpret_cast<int16_t *>(cursor)};
                break;
            case sizeof(int32_t):
                value = Datum{*reinterpret_cast<int32_t *>(cursor)};
                break;
            case sizeof(int64_t):
                value = Datum{*reinterpret_cast<int64_t *>(cursor)};
                break;
            default:
                assert(false);
                throw std::runtime_error("unknown type length " + std::to_string(length));
        }
    } else {
        value = Datum{cursor};

        // First field contains length of remaining data
        auto dataLen = *reinterpret_cast<int32_t *>(cursor);
        length = sizeof(int32_t) + static_cast<size_t>(dataLen);
    }
    return std::make_pair(value, length);
}

void HeapTuple::parseTuple() {
    auto natts = this->_descriptor->GetMaxAttrNumber();
    auto values = std::vector<table::Datum>(natts);
    auto isnull = std::vector<bool>(natts);

    // First process all nulls
    auto tupdata = reinterpret_cast<char *>(this->_tuple.get()) + this->_tuple->hoff;

    auto hasnull = (this->_tuple->flags & HeapTupleFlags::HasNulls) != 0;
    if (hasnull) {
        auto bitmap = reinterpret_cast<char *>(this->_tuple.get()) + SizeofHeapPageTupleHeader;
        for (auto attno = AttrNumber{AttrNumber::Min}; attno <= natts; attno++) {
            auto num = attno.ToIndex();
            isnull[num] = !(bitmap[num >> 3] & (1 << (num & 0x07)));
        }
    } else {
        std::fill(isnull.begin(), isnull.end(), false);
    }
    
    // Now parse actual data
    tupdata = reinterpret_cast<char *>(this->_tuple.get()) + this->_tuple->hoff;
    auto attributes = this->_descriptor->Attributes();
    for (auto attno = AttrNumber{AttrNumber::Min}; attno <= natts; attno++) {
        auto num = attno.ToIndex();
        if (hasnull && isnull[num]) {
            // already set
            continue;
        }

        auto &desc = attributes[num];

        auto [value, length] = extract_attr_datum(tupdata, desc);
        values[num] = value;
        tupdata += length;
    }
}

std::optional<table::Datum> HeapTuple::GetAttribute(table::AttrNumber attno) {
    if (this->_descriptor->GetMaxAttrNumber() < attno) {
        throw std::runtime_error("provided attribute number greater than available");
    }

    if (!this->_processed) {
        this->parseTuple();
    }

    if (this->_isnull[attno.ToIndex()]) {
        return std::nullopt;
    } else {
        return std::make_optional(this->_values[attno.ToIndex()]);
    }
}
