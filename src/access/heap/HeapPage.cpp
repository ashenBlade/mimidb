#include "mimidb.hpp"

#include "access/heap/HeapPage.hpp"
#include <cassert>

using namespace mi::access::heap;

HeapPage::HeapPage(std::byte *buffer): _buffer(buffer) {
    assert(buffer != nullptr);
};

static HeapPageHeader *cast_header(std::byte *bytes) {
    return reinterpret_cast<HeapPageHeader *>(bytes);
}

const HeapPageHeader& HeapPage::GetHeader() const {
    return *cast_header(_buffer);
};

HeapPageHeader& HeapPage::GetHeader() {
    return *cast_header(_buffer);
};

static uint16_t page_nitems(HeapPageHeader *header) {
    auto length = header->lower - offsetof(HeapPageHeader, items);
    return static_cast<uint16_t>(length / sizeof(ItemId));
}

uint16_t HeapPage::ItemsCount() const {
    auto header = cast_header(_buffer);
    return page_nitems(header);
}

static ItemId *header_get_itemid(HeapPageHeader *header, int index) {
    assert(index < page_nitems(header));
    return &header->items[index];
}

ItemId &HeapPage::GetItemId(int index) {
    auto header = cast_header(_buffer);
    return *header_get_itemid(header, index);
}

const ItemId &HeapPage::GetItemId(int index) const {
    auto header = cast_header(_buffer);
    return *header_get_itemid(header, index);
}

static HeapPageTupleHeader *header_get_tuple(std::byte *buffer, const ItemId &itemId) {
    assert(itemId.isNormal());
    auto offset = itemId.getOffset();
    assert(offset < mi::PAGESIZE);
    assert(sizeof(HeapPageHeader) < offset);
    return reinterpret_cast<HeapPageTupleHeader *>(buffer + offset);
}

HeapPageTupleHeader *HeapPage::GetTuple(const ItemId &itemId) {
    return header_get_tuple(_buffer, itemId);
}

const HeapPageTupleHeader *HeapPage::GetTuple(const ItemId &itemId) const {
    return header_get_tuple(_buffer, itemId);
}

