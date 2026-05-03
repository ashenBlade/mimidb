#pragma once

#include "transam/Transaction.hpp"
#include "worker/Worker.hpp"

namespace mi {
    // Worker object for this thread
    extern thread_local mi::worker::Worker *MyWorker;
    // Current transaction state
    extern thread_local mi::transam::Transaction *MyTransaction;
}
