#pragma once

#include <cstdint>
// for std::hash<>
#include <unordered_map>

namespace mi::storage::trans {

/// @brief Transaction Identifier
struct TransactionId final {
    // Underlying XID scalar type
    using type = uint64_t;

    uint64_t value;

    TransactionId() : value(Invalid) {};
    TransactionId(uint64_t xid) : value(xid) {};
    operator uint64_t() const { return value; }

    // TransactionId is not Invalid value
    bool isValid() const { return value != TransactionId::Invalid; }

    // TransactionId value is in valid range, between Min and Max
    bool isNormal() const { return TransactionId::Min <= value && value <= TransactionId::Max; }

    // Invalid value of XID
    static const constexpr uint64_t Invalid = 0;
    // Minimal usable XID value (0 is Invalid)
    static const constexpr uint64_t Min = 1;
    // Maximal value of XID.
    // 2 highest bits are used by others, i.e. clog uses for committed/aborted
    static const constexpr uint64_t Max = UINT64_MAX >> 2;
};
}; // namespace mi::transam

namespace std {
template <> struct hash<mi::storage::trans::TransactionId> {
    size_t operator()(const mi::storage::trans::TransactionId &xid) const {
        return std::hash<mi::storage::trans::TransactionId::type>()(xid.value);
    }
};
} // namespace std