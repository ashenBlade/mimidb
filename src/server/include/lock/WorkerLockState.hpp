#pragma once

namespace mi::lock {

// Different states for worker when working with locks
enum class WorkerLockState {
    NotWaiting,     // Not waiting, woken up
    Waiting,        // Waiting for lock
    PendingWakeup,  // Removed from waitlist, but not yet woken up
};
}
