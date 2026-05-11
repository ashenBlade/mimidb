#pragma once

#include "executor/Oid.hpp"
#include "storage/buffer/Buffer.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageTag.hpp"
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace mi::storage::buffer {
class BufferManager {
  private:
    /// @brief Mapping between page identifier and it's object
    std::unordered_map<PageTag, std::shared_ptr<Buffer>, PageTagHash> _map;
    /// @brief For for updating pages array
    std::shared_mutex _mutex;

  public:
    BufferManager();

    /// @brief Get buffer for given page number
    /// @param relid Id of relation to get page
    /// @param pageno Number of page to get
    /// @return Buffer entry to get access to buffer and it's contents
    BufferPin GetBuffer(PageTag tag);

    /// @brief Return buffer to pool
    /// @param buffer Buffer to return
    /// @param tag Identifier of page
    void ReturnBuffer(BufferPin &buffer);

    /// @brief Write buffer to disk
    void FlushBuffer(BufferPin &buffer);

    /// @brief Add 1 new page to relation at the end
    /// @return Number of newly allocated and written page
    BufferPin ExtendRelation(Oid relid);

    ~BufferManager();
};
}; // namespace mi::storage::buffer
