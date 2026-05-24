#pragma once

#include <cstddef>

#include "access/heap/HeapPageHeader.hpp"
#include "access/heap/HeapPageTupleHeader.hpp"
#include "access/heap/ItemId.hpp"

namespace mi::access::heap {
// Wrapper class allowing operations with heap page
class HeapPage final {
  private:
    // Byte array with page contents
    std::byte *_buffer;

  public:
    HeapPage(std::byte *buffer);

    HeapPageHeader &GetHeader();
    const HeapPageHeader &GetHeader() const;

    /// @brief Get number of items on given page
    uint16_t ItemsCount() const;
    /// @brief Get ItemId at specified index
    /// @param index Index of ItemId, 0-based
    /// @return Pointer to ItemId
    ItemId &GetItemId(int index);
    const ItemId &GetItemId(int index) const;

    /// @brief Get pointer to start of line pointer array
    ItemId *GetLinePointerArray();
    const ItemId *GetLinePointerArray() const;

    /// @brief Get pointer to tuple to which ItemId is pointing
    /// @param itemId Item id for this tuple, must be valid
    /// @return Pointer to tuple
    HeapPageTupleHeader *GetTuple(const ItemId &itemId);
    const HeapPageTupleHeader *GetTuple(const ItemId &itemId) const;

    size_t GetFreeSpace() const;
    
    bool IsNew() const;
    
    // Initialize newly create page (filled with zeroes)
    static void Init(HeapPage &page);
};

}; // namespace mi::access::heap