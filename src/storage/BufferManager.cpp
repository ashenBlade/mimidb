#include "mimidb.hpp"
#include "storage/BufferManager.hpp"
#include "access/table/Oid.hpp"
#include "storage/BufferPin.hpp"
#include "storage/RelFile.hpp"
#include "storage/PageTag.hpp"

#include <array>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <format>
#include <fcntl.h>

using namespace mi::storage;
using namespace mi;

BufferManager::BufferManager() : _map(), _mutex() {}

static RelFile open_relation(Oid relid, int mode) {
    auto filepath = std::format("data/{}", relid.value);
    return RelFile{File::Open(filepath, mode)};
}

BufferPin BufferManager::GetBuffer(PageTag tag) {
    auto lock = std::shared_lock{this->_mutex};
    auto it = this->_map.find(tag);
    if (it != this->_map.end()) {
        // Ok. Found existing page.
        return BufferPin{tag, it->second};
    }
    // Page not found - read it from disk.

    // Now we have to update our lock to X to synchronize access for both map and IO
    lock.release();
    auto guard = std::lock_guard{this->_mutex};

    // Open file and read page into stack allocated buffer
    auto file = open_relation(tag.Relid, O_RDONLY);
    auto data = std::array<std::byte, PAGESIZE>{};
    file.Read(data.data(), tag.PageNo);

    // Success. Allocate byte array in heap and create Buffer
    auto d = new std::byte[PAGESIZE];
    std::copy(data.data(), data.data() + PAGESIZE, d);
    auto buffer = std::make_shared<Buffer>(d);

    // Add new entry
    this->_map.emplace(tag, buffer);
    return BufferPin{tag, buffer};
}

void BufferManager::ReturnBuffer([[maybe_unused]] BufferPin &pin) {
    // Пока ничего - все в памяти, без вытеснения
}

void BufferManager::FlushBuffer([[maybe_unused]] BufferPin &pin) {
    // Пока ничего - все в памяти, без вытеснения
}

BufferPin BufferManager::ExtendRelation(Oid relid) {
    // For now for this operation use single lock for whole buffer manager.
    // This locks both internal map and 
    auto lock = std::lock_guard{this->_mutex};
    auto file = open_relation(relid, O_WRONLY);

    // Get amount of pages so far and decide which page will be new
    auto npages = file.GetPagesCount();

    // Page numbers inclusive
    auto newPageno = npages;
    file.Extend(newPageno);

    // Now on disk page exists and zeroed - create Buffer for it
    auto data = new std::array<std::byte, PAGESIZE>{};
    std::fill(data->begin(), data->end(), static_cast<std::byte>(0));
    auto buffer = std::make_shared<Buffer>(data->data());

    // Add it to the map
    auto tag = PageTag{relid, newPageno};
    this->_map.emplace(tag, buffer);

    return BufferPin{tag, buffer};
}
