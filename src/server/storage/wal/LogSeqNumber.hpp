#pragma once

#include <cstdint>

namespace mi::storage::wal {

// Log Sequence Number describing address of record in WAL
struct LogSeqNumber final {
    static constexpr const uint64_t InvalidValue = 0;
    uint64_t value;

    LogSeqNumber() : value(InvalidValue) {};
    LogSeqNumber(uint64_t value) : value(value) {};

    // Get numeric value of LSN
    operator uint64_t() const { return value; }
    
    constexpr bool IsValid() const {
        return this->value != InvalidValue;
    }

    static LogSeqNumber Invalid() {
        return LogSeqNumber{InvalidValue};
    }
};

}; // namespace mi::transam