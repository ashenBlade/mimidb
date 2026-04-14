#pragma once

#include <cstddef>

#include "lock/LWLatch.hpp"

namespace mi::storage {

/// @brief Represents page in buffer pool
class Buffer {
private:
    // Page contents 
    std::byte *_contents;

    // Latch for contents
    lock::LWLatch _latch;

    // Page is dirty
    bool _dirty;
public:
    Buffer(std::byte *contents);

    std::byte *GetContents();
    const std::byte *GetContents() const;
    void Lock(bool shared);
    void Unlock(bool shared);
};
}; // namespace mi::storage
