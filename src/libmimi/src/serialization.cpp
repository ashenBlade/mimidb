#include "NetworkReader.hpp"
#include "NetworkWriter.hpp"
#include "packets/CommandCompletePacket.hpp"
#include "packets/DataRowPacket.hpp"
#include "packets/SerializerPacketVisitor.hpp"
#include "packets/PacketDeserializer.hpp"
#include "packets/ErrorResponsePacket.hpp"
#include "packets/TupleDescriptionPacket.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <endian.h>

using namespace mi::interface::libmimi;

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
std::vector<std::byte> visit_base(const TPacket &packet, NetworkWriter *writer_out) {
    auto size = calculate_payload_size(packet);
    auto buffer = std::vector<std::byte>(size + sizeof(std::byte) + sizeof(uint32_t));

    auto writer = NetworkWriter{buffer.data(), buffer.size()};

    // Common header: type + length
    writer.WriteUint8(static_cast<uint8_t>(packet.Type()));
    // Length include payload and 'length' field itself (4 bytes)
    writer.WriteUint32(static_cast<uint32_t>(size + sizeof(uint32_t)));

    *writer_out = writer;
    return buffer;
}

void SerializerPacketVisitor::Visit(const QueryPacket &packet) {
    // Calculate payload size
    NetworkWriter writer;
    auto buffer = visit_base(packet, &writer);

    writer.WriteString(packet.Query());
    this->_buffer = std::move(buffer);
}


QueryPacket PacketDeserializer::DeserializeQuery(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};
    auto query = reader.ReadString();
    return QueryPacket{std::move(query)};
};

void SerializerPacketVisitor::Visit(const TupleDescriptionPacket &packet) {
    // Calculate payload size
    NetworkWriter writer;
    auto buffer = visit_base(packet, &writer);

    // Number of attributes
    writer.WriteUint32(static_cast<uint32_t>(packet.Attributes().size()));

    for (const auto &att : packet.Attributes()) {
        writer.WriteString(att.Name());
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
    NetworkWriter writer;
    auto buffer = visit_base(packet, &writer);

    // Number of attributes
    writer.WriteUint32(static_cast<uint32_t>(packet.Values().size()));

    for (const auto &att : packet.Values()) {
        if (!att.has_value()) {
            writer.WriteUint32(static_cast<uint32_t>(-1));
            continue;
        }

        const auto &val = att.value();
        writer.WriteString(val);
        assert(static_cast<uint32_t>(val.size()) != static_cast<uint32_t>(-1));
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
    NetworkWriter writer;
    auto buffer = visit_base(packet, &writer);

    writer.WriteString(packet.Message());

    this->_buffer = std::move(buffer);
}

ErrorResponsePacket PacketDeserializer::DeserializeErrorResponse(std::byte *buffer, size_t length) {
    auto reader = NetworkReader{buffer, length};
    auto msg = reader.ReadString();
    return ErrorResponsePacket{std::move(msg)};
};

void SerializerPacketVisitor::Visit(const CommandCompletePacket &packet) {
    NetworkWriter writer;
    auto buffer = visit_base(packet, &writer);
    // No fields
    this->_buffer = std::move(buffer);
}

CommandCompletePacket PacketDeserializer::DeserializeCommandComplete([[maybe_unused]] std::byte *buffer, [[maybe_unused]] size_t length) {
    return CommandCompletePacket{};
}
