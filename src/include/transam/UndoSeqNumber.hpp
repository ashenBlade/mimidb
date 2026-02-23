#pragma once

#include <cstdint>

namespace mi::transam {

/// @brief Undo Sequence Number describing location of record in undo log
struct UndoSeqNumber final {
    /// @brief Actual value of USN
    uint64_t usn;

    UndoSeqNumber(uint64_t value): usn(value) { };
    /// @brief Get numeric value of USN
    operator uint64_t() const { return usn; };
    
    static constexpr const uint64_t Max = UINT64_MAX >> 2;
};

}; // namespace mi::transam