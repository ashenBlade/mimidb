#pragma once

#include <cstdint>
#include <cassert>

namespace mi::access::heap {

enum ItemState {
    Unused = 0,     // free, not used
    Normal,         // contains tuple
    Dead,           // updated, but new tuple moved to another page
};

// Pointer to tuple on heap page
struct ItemId final {
    /// @brief State of tuple, stored as ItemState value
    ItemState flags : 2;
    /// @brief Offset in bytes to tuple
    uint32_t offset : 15;
    /// @brief Length of tuple
    uint32_t length : 15; // length of item
    
    bool isUnused() const { return flags == ItemState::Unused; }
    bool isNormal() const { return flags == ItemState::Normal; }
    bool isDead() const { return flags == ItemState::Dead; }

    /// This item have tuple header on page
    bool hasHeader() const { return flags == ItemState::Normal || flags == ItemState::Dead; }
    uint16_t getLength() const { return static_cast<uint16_t>(length); }
    uint16_t getOffset() const { return static_cast<uint16_t>(offset); }
    
    void setLength(uint16_t length) {
        this->length = length & 0x7FFF;
        assert(this->length == length);
    };
    
    void setOffset(uint16_t offset) {
        this->offset = offset & 0x7FFF;
        assert(this->offset == offset);
    }
    
    void setDead() {
        this->flags = ItemState::Dead;
        this->offset = 0;
        this->length = 0;
    }
};

}; // namespace mi::access::heap