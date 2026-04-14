#include "mimidb.hpp"

#include "worker/Worker.hpp"

using namespace mi::worker;

Worker::Worker() : Worker(WorkerId::Invalid) {};

Worker::Worker(WorkerId id) : _id(id), _lockState(), _sema() { assert(id.IsValid()); };

Worker::Worker(Worker &&other) noexcept { this->swap(other); }

void Worker::swap(Worker &other) noexcept {
    std::swap(this->_id, other._id);
    std::swap(this->_lockState, other._lockState);
    std::swap(this->_sema, other._sema);
}

Worker &Worker::operator=(Worker &&other) noexcept {
    this->swap(other);
    return *this;
}

WorkerId Worker::GetId() const { return this->_id; }

mi::lock::LockState &Worker::GetLockState() {
    return this->_lockState;
}

mi::lock::Semaphore &Worker::GetSemaphore() {
    assert(this->_sema != nullptr);
    return *this->_sema;
}
