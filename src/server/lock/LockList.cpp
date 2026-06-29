#include "lock/LockList.hpp"
#include "cluster_state.hpp"
#include "worker/WorkerId.hpp"

using namespace mi::lock;

static LockListNode &get_node(mi::worker::WorkerId id) {
    auto worker = mi::LockGlobal->GetWorkerLock(id);
    return worker->GetLockNode();
}

void LockListHead::PushTail(mi::worker::WorkerId id) {
    auto &node = get_node(id);
    if (this->Tail == worker::WorkerId::Invalid()) {
        // List is empty, so insert single element
        node.Next = node.Prev = worker::WorkerId::Invalid();
        this->Head = this->Tail = id;
    } else {
        // Insert new node after old last element
        node.Prev = this->Tail;
        node.Next = worker::WorkerId::Invalid();

        // Make old tail points next to us
        get_node(node.Prev).Next = id;

        // Update tail
        this->Tail = id;
    }
}

void LockListHead::Delete(worker::WorkerId id) {
    auto &node = get_node(id);

    if (node.Prev == worker::WorkerId::Invalid()) {
        assert(this->Head == id);
        this->Head = node.Next;
    } else {
        get_node(node.Prev).Next = node.Next;
    }

    if (node.Next == worker::WorkerId::Invalid()) {
        assert(this->Tail == id);
        this->Tail = node.Prev;
    } else {
        get_node(node.Next).Prev = node.Prev;
    }

    // Invalidate deleted node
    node.Next = node.Prev = worker::WorkerId::Invalid();
}

LockListHead::LockListIterator &LockListHead::LockListIterator::operator++() {
    this->Current = this->Next;
    if (this->Current != worker::WorkerId::Invalid()) {
        this->Next = get_node(this->Current).Next;
    }

    return *this;
}

LockListHead::LockListIterator LockListHead::begin() {
    auto start = this->Head;
    auto next = this->Head.IsValid() ? get_node(this->Head).Next : worker::WorkerId::Invalid();
    return LockListHead::LockListIterator{start, next};
}

