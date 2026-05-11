#pragma once

#include "worker/WorkerId.hpp"

namespace mi::worker {
    /// @brief Main handler for user connection
    /// @param workerId Id of worker to which handler belongs
    /// @param sock Socket for user connection
    extern void HandleUserConnection(WorkerId workerId, int sock);
}

