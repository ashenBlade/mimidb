#pragma once

#include <cstdint>

namespace mi::access::heap {

enum class ItemState {
    Unused = 0, // not used
    Normal, // normal
    Dead,   // dead, should not be used
};

// Pointer to tuple on heap page
struct ItemId final {
    /// @brief State of tuple, stored as ItemState value
    ItemState flags : 2;
    /// @brief Offset in bytes to tuple
    uint32_t offset : 15;
    /// @brief Length of tuple
    uint32_t length : 15; // length of item

    bool isNormal() const { return flags == ItemState::Normal; }
    bool isUnused() const { return flags == ItemState::Unused; }
    bool isDead() const { return flags == ItemState::Dead; }
    uint16_t getLength() const { return static_cast<uint16_t>(length); }
    uint16_t getOffset() const { return static_cast<uint16_t>(offset); }
};

}; // namespace mi::access::heap