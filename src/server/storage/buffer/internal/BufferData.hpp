#pragma once

#include "adt/LinkedList.hpp"
#include "lock/LWLatch.hpp"
#include <condition_variable>
#include <memory>
namespace mi::storage::buffer {

// Data required to work with buffer.
// Stored separately from CacheEntry to reduce memory consumption.
struct BufferData : public adt::LinkedListEntry<BufferData> {
    // Pointer to page bytes
    std::unique_ptr<std::byte[]> Page;

    // Content lock
    lock::LWLatch Latch;

    // Condition variable to perform IO
    std::condition_variable_any IOCondVar;
};

} // namespace mi::storage::buffer
