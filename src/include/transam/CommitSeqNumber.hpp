#pragma once

#include <cstdint>

namespace mi::transam {

struct CommitSeqNumber {
    uint64_t csn;

    CommitSeqNumber(uint64_t csn) : csn(csn) {};
    operator uint64_t() { return csn; };
    
    bool isInvalid() const { return csn == CommitSeqNumber::Invalid; };

    /// @brief Maximal value for CSN
    static constexpr const uint64_t Max = UINT64_MAX >> 2;
    /// @brief Start value for CSN
    static constexpr const uint64_t Min = 1;
    /// @brief Invalid value for CSN
    static constexpr const uint64_t Invalid = 0;
};

}; // namespace mi::transam
