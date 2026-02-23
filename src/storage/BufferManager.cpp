#include "storage/BufferManager.hpp"

#include <stdexcept>
#include <shared_mutex>
#include <mutex>

using namespace mi::storage;

mi::storage::BufferManager::BufferManager() : _pages(), _mutex() {}

std::shared_ptr<Buffer> mi::storage::BufferManager::GetBuffer(PageNumber pageno) {
    auto lock = std::shared_lock(_mutex);

    // If page already in cache - return it
    if (pageno < _pages.size()) {
        return _pages[pageno];
    }
    
    throw std::runtime_error("not implemented");
}