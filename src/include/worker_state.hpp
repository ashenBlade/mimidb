#pragma once

#include "worker/Worker.hpp"
#include "db/Database.hpp"

namespace mi {
    // Worker object for this thread
    extern thread_local mi::worker::Worker *MyWorker;

    // Database for this worker
    extern mi::db::Database *MyDatabase;
}
