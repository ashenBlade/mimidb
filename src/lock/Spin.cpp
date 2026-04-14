#include "mimidb.hpp"

#include <chrono>
#include <thread>

#include "lock/Spin.hpp"

using namespace mi::lock;

void Spin::PerformSpin() {
    // just sleep as in pg, but do not account in spin stuck
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}
