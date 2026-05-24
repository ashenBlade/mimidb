#pragma once

#include "trans/Transaction.hpp"
#include "worker/Worker.hpp"

namespace mi {
// Worker object for this thread
extern thread_local mi::worker::Worker *MyWorker;
// Current transaction state
extern thread_local mi::storage::trans::Transaction *MyTransaction;
} // namespace mi
