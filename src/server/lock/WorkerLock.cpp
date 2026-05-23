#include "lock/WorkerLock.hpp"

using namespace mi::lock;

WorkerLock::WorkerLock(): _sema(1), _lockState(), _lockMode(), _lockNode() {};

