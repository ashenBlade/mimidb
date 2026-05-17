#include "NetworkReader.hpp"
#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/SerializerPacketVisitor.hpp"
#include "packets/PacketDeserializer.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <endian.h>

using namespace mi::interface::libmimi;

// TODO: переделать на NetworkWriter
static size_t calculate_payload_size(const QueryPacket &packet) {
    // common header
    size_t size = 0;

    // Query length
    size += sizeof(uint32_t);
    size += packet.Query().size();

    return size;
}


static size_t calculate_payload_size(const TupleDescriptionPacket &packet) {
    size_t size = 0;

    // Number of attributes
    size += sizeof(uint32_t);

    // Calculate each attribute
    for (const auto &att : packet.Attributes()) {
        size += sizeof(uint32_t);
        size += att.Name().size();
    }

    return size;
}

static size_t calculate_payload_size(const DataRowPacket &packet) {
    size_t size = 0;

    // Number of attributes
    size += sizeof(uint32_t);

    // Calculate each attribute
    for (const auto &att : packet.Values()) {
        if (att.has_value()) {
            // String length
            size += sizeof(uint32_t);
            // Data length
            size += att.value().size();
        } else {
            // NULL value (-1)
            size += sizeof(uint32_t);
        }
    }

    return size;
}

static size_t calculate_payload_size(const ErrorResponsePacket &packet) {
    size_t size = 0;

    size += sizeof(uint32_t);
    size += packet.Message().size();

    return size;
}

static size_t calculate_payload_size([[maybe_unused]]const CommandCompletePacket &packet) {
    size_t size = 0;

    // There is no actual payload

    return size;
}

template<class TPacket>
std::vector<std::byte> visit_base(const TPacket &packet, std::byte **cursor_out) {
    auto size = calculate_payload_size(packet);
    auto buffer = std::vector<std::byte>(size + sizeof(std::byte) + sizeof(uint32_t));

    auto cursor = buffer.data();
    
    *reinterpret_cast<std::byte *>(cursor) = static_cast<std::byte>(packet.Type());
    cursor += sizeof(std::byte);

    *reinterpret_cast<uint32_t *>(cursor) = be32toh(static_cast<uint32_t>(size + sizeof(uint32_t)));
    cursor += sizeof(uint32_t);

    *cursor_out = cursor;

    return buffer;
}

void SerializerPacketVisitor::Visit(const QueryPacket &packet) {
    // Calculate payload size
    std::byte *cursor;
    auto buffer = visit_base(packet, &cursor);

    *reinterpret_cast<uint32_t *>(cursor) = be32toh(static_cast<uint32_t>(packet.Query().size()));
    cursor += sizeof(uint32_t);

    std::memcpy(cursor, packet.Query().c_str(), packet.Query().size());

    this->_buffer = std::move(buffer);
}


QueryPacket PacketDeserializer::DeserializeQuery(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};
    auto query = reader.ReadString();
    return QueryPacket{std::move(query)};
};

void SerializerPacketVisitor::Visit(const TupleDescriptionPacket &packet) {
    // Calculate payload size
    std::byte *cursor;
    auto buffer = visit_base(packet, &cursor);

    // Number of attributes
    *reinterpret_cast<uint32_t *>(cursor) = be32toh(static_cast<uint32_t>(packet.Attributes().size()));
    cursor += sizeof(uint32_t);

    for (const auto &att : packet.Attributes()) {
        *reinterpret_cast<uint32_t *>(cursor) = be32toh(static_cast<uint32_t>(att.Name().size()));
        cursor += sizeof(uint32_t);

        std::memcpy(cursor, att.Name().data(), att.Name().size());
        cursor += att.Name().size();
    }

    this->_buffer = std::move(buffer);
}

TupleDescriptionPacket PacketDeserializer::DeserializeTupleDescription(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};
    auto nattrs = reader.ReadUint32();
    auto attrs = std::vector<AttributeDescription>{};
    for (auto i = 0U; i < nattrs; ++i) {
        auto name = reader.ReadString();
        attrs.emplace_back(std::move(name));
    }

    return TupleDescriptionPacket{std::move(attrs)};
};

void SerializerPacketVisitor::Visit(const DataRowPacket &packet) {
    std::byte *cursor;
    auto buffer = visit_base(packet, &cursor);

    // Number of attributes
    *reinterpret_cast<uint32_t *>(cursor) = htobe32(static_cast<uint32_t>(packet.Values().size()));
    cursor += sizeof(uint32_t);

    for (const auto &att : packet.Values()) {
        if (!att.has_value()) {
            *reinterpret_cast<uint32_t *>(cursor) = static_cast<uint32_t>(-1);
            cursor += sizeof(uint32_t);
            continue;
        }

        const auto &val = att.value();
        *reinterpret_cast<uint32_t *>(cursor) = htobe32(static_cast<uint32_t>(val.size()));
        cursor += sizeof(uint32_t);

        std::memcpy(cursor, val.data(), val.size());
        cursor += val.size();
    }

    this->_buffer = std::move(buffer);
}

DataRowPacket PacketDeserializer::DeserializeDataRow(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};

    auto natts = reader.ReadUint32();
    auto values = std::vector<std::optional<std::string>>{};

    for (auto i = 0U; i < natts; ++i) {
        auto value = reader.ReadUint32();
        if (value == static_cast<uint32_t>(-1)) {
            values.emplace_back(std::nullopt);
        } else {
            auto buf = std::string(value, 0);
            reader.ReadBuffer(value, reinterpret_cast<std::byte *>(buf.data()));
            values.emplace_back(std::move(buf));
        }
    }

    return DataRowPacket{std::move(values)};
};

void SerializerPacketVisitor::Visit(const ErrorResponsePacket &packet) {
    std::byte *cursor;
    auto buffer = visit_base(packet, &cursor);

    *reinterpret_cast<uint32_t *>(cursor) = htobe32(static_cast<uint32_t>(packet.Message().size()));
    cursor += sizeof(uint32_t);

    std::memcpy(cursor, packet.Message().data(), packet.Message().size());

    this->_buffer = std::move(buffer);
}

ErrorResponsePacket PacketDeserializer::DeserializeErrorResponse(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};
    auto msg = reader.ReadString();
    return ErrorResponsePacket{std::move(msg)};
};

void SerializerPacketVisitor::Visit(const CommandCompletePacket &packet) {
    std::byte *cursor;
    auto buffer = visit_base(packet, &cursor);
    this->_buffer = std::move(buffer);
}


CommandCompletePacket PacketDeserializer::DeserializeCommandComplete([[maybe_unused]] std::byte *buffer, [[maybe_unused]] size_t length) {
    return CommandCompletePacket{};
}
