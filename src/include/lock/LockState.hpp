#pragma once

#include <cstdint>

#include "lock/LockMode.hpp"
#include "worker/WorkerList.hpp"

namespace mi::lock {
enum class LockWaitState : std::int_fast8_t {
    // Not waiting/woken up
    NoWait,
    // Waiting
    Wait,
    // Removed from wait list, but not yet signaled
    Pending,
};

struct LockState {
    /// @brief Whether this is waiting or not
    LockWaitState status;
    /// @brief Requested lock mode
    LockMode mode;
    /// @brief Wait list node
    mi::worker::WorkerListNode waiters;

    LockState() : status(LockWaitState::NoWait), mode(LockMode::Share), waiters() {};
};
} // namespace mi::lock