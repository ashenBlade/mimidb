#pragma once

#include <cstdint>

namespace mi::transam {

/// @brief Log Sequence Number describing address of record in WAL or UNDO
struct LogSeqNumber final {
    uint64_t lsn;

    LogSeqNumber(uint64_t value): lsn(value) { };
    /// @brief Get numeric value of LSN
    operator uint64_t() const { return lsn; }
};

}; // namespace mi::transam