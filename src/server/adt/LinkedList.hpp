#pragma once

#include <atomic>
#include <type_traits>

namespace mi::adt {

// Forward declarations
template <class T> class LinkedListEntry;

template <class T, std::enable_if_t<std::is_convertible_v<T *, LinkedListEntry<T> *>, bool>>
class LinkedList;

// Single entry of linked list
template <class T> class LinkedListEntry {
  public:
    T *Next{nullptr};
};

// Concurrent linked list implemented using atomic operations.
// All entries must derive from LinkedListEntry.
template <class T, std::enable_if_t<std::is_convertible_v<T *, LinkedListEntry<T> *>, bool> = true>
class LinkedList {
  private:
    // Head of linked list
    std::atomic<T *> _head{nullptr};

    using ListEntry = LinkedListEntry<T>;

  public:
    void Add(T *node) {
        auto old = this->_head.load();
        do {
            // Update pointer to next entry
            static_cast<ListEntry *>(node)->Next = old;
        } while (!this->_head.compare_exchange_weak(old, node, std::memory_order_release,
                                                    std::memory_order_acquire));
    }

    T *Get() {
        auto old = this->_head.load();
        do {
            if (!old) {
                // List is empty
                return nullptr;
            }

        } while (!this->_head.compare_exchange_weak(old, old->Next, std::memory_order_acquire,
                                                    std::memory_order_relaxed));

        return old;
    }
};

}; // namespace mi::adt
