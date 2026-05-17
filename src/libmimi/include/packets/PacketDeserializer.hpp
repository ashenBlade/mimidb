#pragma once

#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/QueryPacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <cstddef>
namespace mi::interface::libmimi {
class PacketDeserializer {
  public:
    static DataRowPacket DeserializeDataRow(std::byte *buffer, size_t length);
    static QueryPacket DeserializeQuery(std::byte *buffer, size_t length);
    static TupleDescriptionPacket DeserializeTupleDescription(std::byte *buffer, size_t length);
    static ErrorResponsePacket DeserializeErrorResponse(std::byte *buffer, size_t length);
    static CommandCompletePacket DeserializeCommandComplete(std::byte *buffer, size_t length);
};
} // namespace mi::interface::libmimi
