#pragma once

#include <cstdint>

namespace mi::transam {

struct CommitSeqNumber {
    using type = uint64_t;

    uint64_t csn;

    CommitSeqNumber() : csn(Invalid) {};
    CommitSeqNumber(uint64_t csn) : csn(csn) {};

    operator uint64_t() const { return csn; };

    constexpr bool IsInvalid() const { return csn == CommitSeqNumber::Invalid; };
    constexpr bool IsFrozen() const { return csn == CommitSeqNumber::Frozen; };
    constexpr bool IsInProgress() const { return csn == CommitSeqNumber::InProgress; };
    constexpr bool IsAborted() const { return csn == CommitSeqNumber::Aborted; };
    constexpr bool IsCommitting() const { return csn == CommitSeqNumber::Committing; };
    constexpr bool IsNormal() const { return csn >= Min; }

    /// @brief Invalid value for CSN
    static constexpr const uint64_t Invalid = 0;
    /// @brief CSN is visible for everyone
    static constexpr const uint64_t Frozen = 1;
    /// @brief Special value for return when transaction is executing and yet not ended
    static constexpr const uint64_t InProgress = 2;
    /// @brief Special value for return when transaction is aborted
    static constexpr const uint64_t Aborted = 3;
    /// @brief Special value for return when transaction is committing right now and yet not
    /// obtained it's CSN
    static constexpr const uint64_t Committing = 4;
    /// @brief Start value for CSN
    static constexpr const uint64_t Min = 5;
    /// @brief Maximal value for CSN
    static constexpr const uint64_t Max = UINT64_MAX >> 1;
};

}; // namespace mi::transam
