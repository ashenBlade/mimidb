#include "mimidb.hpp"

#include "worker_state.hpp"

using namespace mi::worker;

WorkerListIterator WorkerListHead::Iterate() {
    return WorkerListIterator{this->head};
}

void WorkerListHead::Push(WorkerId id) {
    auto &newWorker = WorkerGlobal->GetWorker(id);
    auto &newNode = newWorker.GetLockState().waiters;

    if (this->tail.IsValid()) {
        newNode.next = WorkerId::Invalid;

        auto &oldTail = WorkerGlobal->GetWorker(this->tail).GetLockState().waiters;
        newNode.prev = this->tail;
        oldTail.next = id;
        this->tail = id;
    } else {
        this->head = this->tail = id;
        newNode.next = newNode.prev = WorkerId::Invalid;
    }
}

void WorkerListHead::Delete(WorkerId id) {
    auto &worker = WorkerGlobal->GetWorker(id);
    auto &node = worker.GetLockState().waiters;

    if (node.prev.IsValid()) {
        auto &prev = WorkerGlobal->GetWorker(node.prev).GetLockState().waiters;
        prev.next = node.next;
    } else {
        assert(this->head == id);
        this->head = node.next;
    }

    if (node.next.IsValid()) {
        auto &next = WorkerGlobal->GetWorker(node.next).GetLockState().waiters;
        next.prev = node.prev;
    } else {
        assert(this->tail == id);
        this->tail = node.prev;
    }

    node.next = node.prev = WorkerId::Invalid;
}

bool WorkerListHead::IsEmpty() const { return !this->head.IsValid(); }

