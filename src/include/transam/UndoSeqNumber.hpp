#pragma once

#include <cstdint>

namespace mi::transam {

/// @brief Undo Sequence Number describing location of record in undo log
struct UndoSeqNumber final {
    /// @brief Actual value of USN
    uint64_t value;

    UndoSeqNumber() : value(Invalid) {};
    UndoSeqNumber(uint64_t value): value(value) { };
    /// @brief Get numeric value of USN
    operator uint64_t() const { return value; };

    bool IsValid() const noexcept { return this->value != Invalid; };

    static constexpr const uint64_t Max = UINT64_MAX >> 2;
    static constexpr const uint64_t Invalid = 0;
};

}; // namespace mi::transam