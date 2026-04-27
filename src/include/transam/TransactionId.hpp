#pragma once

#include <cstdint>
#include <unordered_map>

namespace mi::transam {

/// @brief Transaction Identifier
struct TransactionId final {
    // Underlying XID scalar type
    using type = uint64_t;

    uint64_t xid;

    TransactionId() : xid(Invalid) {};
    TransactionId(uint64_t xid) : xid(xid) {};
    operator uint64_t() const { return xid; }

    // TransactionId is not Invalid value
    bool isValid() const { return xid != TransactionId::Invalid; }

    // TransactionId value is in valid range, between Min and Max
    bool isNormal() const { return TransactionId::Min <= xid && xid <= TransactionId::Max; }

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
template <> struct hash<mi::transam::TransactionId> {
    using type = mi::transam::TransactionId::type;
    size_t operator()(const mi::transam::TransactionId &xid) {
        return std::hash<type>()(static_cast<type>(xid.xid));
    }
};
} // namespace std