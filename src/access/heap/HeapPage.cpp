#include "mimidb.hpp"

#include "access/heap/HeapPage.hpp"

using namespace mi::access::heap;

const HeapPageHeader& HeapPage::getHeader() const {
    return *reinterpret_cast<HeapPageHeader *>(_buffer);
};

HeapPageHeader& HeapPage::getHeader() {
    return *reinterpret_cast<HeapPageHeader *>(_buffer);
};
