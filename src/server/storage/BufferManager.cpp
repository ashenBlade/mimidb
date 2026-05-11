#include "storage/buffer/BufferManager.hpp"
#include "access/table/Oid.hpp"
#include "mi_config.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageTag.hpp"
#include "storage/buffer/RelFile.hpp"
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <shared_mutex>

using namespace mi::storage::buffer;

BufferManager::BufferManager() : _map(), _mutex() {}

BufferPin BufferManager::GetBuffer(PageTag tag) {
    auto lock = std::shared_lock{this->_mutex};
    auto it = this->_map.find(tag);
    if (it != this->_map.end()) {
        // Ok. Found existing page.
        return BufferPin{tag, it->second};
    }
    // Page not found - read it from disk.

    // Now we have to update our lock to X to synchronize access for both map and IO
    lock.unlock();
    auto guard = std::lock_guard{this->_mutex};

    // Open file and read page into stack allocated buffer
    auto file = RelFile::Open(tag.Relid, O_RDONLY);
    auto data = std::array<std::byte, Config::PageSize>{};
    file.Read(data.data(), tag.PageNo);

    // Success. Allocate byte array in heap and create Buffer
    auto d = new std::byte[Config::PageSize];
    std::copy(data.data(), data.data() + Config::PageSize, d);
    auto buffer = std::make_shared<Buffer>(d);

    // Add new entry
    this->_map.emplace(tag, buffer);
    return BufferPin{tag, buffer};
}

void BufferManager::ReturnBuffer([[maybe_unused]] BufferPin &pin) {
    // Пока ничего - все в памяти, без вытеснения
    assert(pin.IsValid());
}

void BufferManager::FlushBuffer([[maybe_unused]] BufferPin &pin) {
    // Пока ничего - все в памяти, без вытеснения
}

BufferPin BufferManager::ExtendRelation(Oid relid) {
    // For now for this operation use single lock for whole buffer manager.
    // This locks both internal map and
    auto lock = std::lock_guard{this->_mutex};
    auto file = RelFile::Open(relid, O_WRONLY);

    // Get amount of pages so far and decide which page will be new
    auto npages = file.GetPagesCount();

    // Page numbers inclusive
    auto newPageno = npages;
    file.Extend(newPageno);

    // Now on disk page exists and zeroed - create Buffer for it
    auto data = new std::array<std::byte, Config::PageSize>{};
    std::fill(data->begin(), data->end(), static_cast<std::byte>(0));
    auto buffer = std::make_shared<Buffer>(data->data());

    // Add it to the map
    auto tag = PageTag{relid, newPageno};
    this->_map.emplace(tag, buffer);

    return BufferPin{tag, buffer};
}
