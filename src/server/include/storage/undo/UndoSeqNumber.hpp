#pragma once

#include <cstdint>

namespace mi::transam {

/// @brief Undo Sequence Number describing location of record in undo log
struct UndoSeqNumber final {
    /// @brief Actual value of USN
    int64_t value;

    UndoSeqNumber() : value(Invalid) {};
    UndoSeqNumber(int64_t value): value(value) { };
    /// @brief Get numeric value of USN
    operator int64_t() const { return value; };

    bool IsValid() const noexcept { 
        return this->value != Invalid && this->value <= Max;
    };

    static constexpr const int64_t Max = UINT64_MAX >> 2;
    static constexpr const int64_t Invalid = 0;
};

}; // namespace mi::transam