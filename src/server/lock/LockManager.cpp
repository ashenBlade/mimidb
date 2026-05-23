#include "lock/LockManager.hpp"
#include <cstddef>

mi::lock::LockManager::LockManager(std::size_t workersCount): _states(workersCount) {}
