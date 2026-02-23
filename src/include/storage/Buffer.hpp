#pragma once

#include <cstddef>
#include <cstdint>
#include <shared_mutex>

namespace mi::storage {

/// @brief Represents page in buffer pool
class Buffer {
private:
    /// @brief Page contents 
    std::byte *_contents;

    /// @brief Lock for contents
    std::shared_mutex _mutex;

    /// @brief Page is dirty
    bool _dirty;
public:
    Buffer(std::byte *contents);

    std::byte *GetContents();
    const std::byte *GetContents() const;
    void Lock(bool shared);
    void Unlock(bool shared);
};
}; // namespace mi::storage
