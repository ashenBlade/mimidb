#include "access/heap/HeapPage.hpp"
#include "access/heap/HeapPageHeader.hpp"
#include "access/heap/ItemId.hpp"
#include "storage/wal/LogSeqNumber.hpp"
#include "mi_config.hpp"
#include <cassert>
#include <cstring>

using namespace mi::access::heap;

HeapPage::HeapPage(std::byte *buffer) : _buffer(buffer) { assert(buffer != nullptr); };

static HeapPageHeader *cast_header(std::byte *bytes) {
    return reinterpret_cast<HeapPageHeader *>(bytes);
}

const HeapPageHeader &HeapPage::GetHeader() const { return *cast_header(_buffer); };

HeapPageHeader &HeapPage::GetHeader() { return *cast_header(_buffer); };

static uint16_t page_nitems(HeapPageHeader *header) {
    auto length = header->lower - SizeOfHeapPageHeader;
    return static_cast<uint16_t>(length / sizeof(ItemId));
}

uint16_t HeapPage::ItemsCount() const {
    auto header = cast_header(_buffer);
    return page_nitems(header);
}

static ItemId *header_get_itemid(HeapPageHeader *header, int index) {
    ItemId *array =
        reinterpret_cast<ItemId *>(reinterpret_cast<char *>(header) + SizeOfHeapPageHeader);
    return &array[index];
}

ItemId &HeapPage::GetItemId(int index) {
    auto header = cast_header(_buffer);
    assert(index < page_nitems(header));
    return *header_get_itemid(header, index);
}

const ItemId &HeapPage::GetItemId(int index) const {
    auto header = cast_header(_buffer);
    assert(index < page_nitems(header));
    return *header_get_itemid(header, index);
}

ItemId *HeapPage::GetLinePointerArray() {
    return reinterpret_cast<ItemId *>(this->_buffer + SizeOfHeapPageHeader);
}

const ItemId *HeapPage::GetLinePointerArray() const {
    return reinterpret_cast<ItemId *>(this->_buffer + SizeOfHeapPageHeader);
}

static HeapPageTupleHeader *header_get_tuple(std::byte *buffer, const ItemId &itemId) {
    assert(itemId.isNormal());
    auto offset = itemId.getOffset();
    assert(offset < mi::Config::PageSize);
    assert(sizeof(HeapPageHeader) < offset);
    return reinterpret_cast<HeapPageTupleHeader *>(buffer + offset);
}

HeapPageTupleHeader *HeapPage::GetTuple(const ItemId &itemId) {
    return header_get_tuple(_buffer, itemId);
}

const HeapPageTupleHeader *HeapPage::GetTuple(const ItemId &itemId) const {
    return header_get_tuple(_buffer, itemId);
}

size_t HeapPage::GetFreeSpace() const {
    auto header = reinterpret_cast<HeapPageHeader *>(this->_buffer);
    return header->upper - header->lower;
}

bool HeapPage::IsNew() const {
    // lower can not be 0
    return this->GetHeader().lower == 0;
}

void HeapPage::Init(HeapPage &page) {
    auto &header = page.GetHeader();
    header.lsn = 0;
    header.lower = SizeOfHeapPageHeader;
    header.upper = Config::PageSize;
}
