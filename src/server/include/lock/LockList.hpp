#pragma once

#include "worker/WorkerId.hpp"
namespace mi::lock {

struct LockListHead {
    worker::WorkerId Head{worker::WorkerId::Invalid()};
    worker::WorkerId Tail{worker::WorkerId::Invalid()};

    // Add new entry to list 
    void PushTail(worker::WorkerId id);
    // Remove worker from list
    void Delete(worker::WorkerId id);

    bool IsEmpty() const {
        return this->Head.value == worker::WorkerId::Invalid();
    }

    struct LockListIterator {
        // Current observed worker id
        worker::WorkerId Current;
        // Next worker id to iterate
        worker::WorkerId Next;
        LockListIterator &operator++();
        bool operator!=(const LockListIterator &other) {
            // This acts as stop condition during iteration and all we have
            // to check is just current node.
            // It allows us to perform modifying iteration (delete current node)
            return this->Current != other.Current;
        }
    };

    LockListIterator begin();
    LockListIterator end() {
        return LockListIterator{worker::WorkerId::Invalid(), worker::WorkerId::Invalid()};
    }
};

struct LockListNode {
    worker::WorkerId Next{worker::WorkerId::Invalid()};
    worker::WorkerId Prev{worker::WorkerId::Invalid()};
};

} // namespace mi::worker
