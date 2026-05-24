#pragma once

#include <cstdint>

namespace mi::interface::libmimi {

enum PacketType : uint8_t {
    // Query string
    Query = 'Q',
    // Statement ended execution
    CommandComplete = 'C',
    // Description of output tuples
    TupleDescription = 'T',
    // Single row data
    DataRow = 'D',
    // Packet with error description
    ErrorResponse = 'E',
};

}
