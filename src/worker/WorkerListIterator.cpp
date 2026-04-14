#include "mimidb.hpp"

#include "worker_state.hpp"

using namespace mi::worker;

WorkerListIterator::WorkerListIterator(WorkerId start)
    // First is Next() invoked, so next shifted to current.
    // Such initialization allows us not to write additional checks
    : _current(WorkerId::Invalid), _next(start) {}
    
bool WorkerListIterator::Next() {
    this->_current = this->_next;

    if (!this->_current.IsValid()) {
        // Both current and next are set to Invalid
        return false;
    }

    // Remember what node is next for now, because we can (and very
    // probably will) change it during iteration.
    auto &worker = WorkerGlobal->GetWorker(this->_current);
    this->_next = worker.GetId();
    return true;
}

Worker &WorkerListIterator::operator*() {
    return WorkerGlobal->GetWorker(this->_current);
}