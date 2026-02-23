#pragma once

#include "access/heap/HeapPageHeader.hpp"
#include "access/heap/ItemId.hpp"

namespace mi::access::heap {

/// @brief Heap page wrapper
class HeapPage final {
  private:
    // Byte array with page contents
    char *_buffer;

  public:
    HeapPage(char *buffer);
    HeapPageHeader& getHeader();
    const HeapPageHeader& getHeader() const;
};

}; // namespace mi::access::heap