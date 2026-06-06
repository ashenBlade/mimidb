#pragma once

#include "lock/LWLatch.hpp"
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mi::adt {
template <class TKey, class TValue, class THash = std::hash<TKey>,
          class TKeyEqual = std::equal_to<TKey>>
class HashTable {
  private:
    using MapType = std::unordered_map<TKey, TValue, THash, TKeyEqual>;

    struct MapLockPair {
        MapType Map;
        lock::LWLatch Latch;
    };

    // Vector of map for each partition
    std::vector<MapLockPair> _maps;
    size_t getElementIndex(const TKey &key) {
        auto hash = THash()(key);
        return hash % this->_maps.size();
    }

  public:
    HashTable(size_t npartitions) : _maps(npartitions) {};

    std::unique_lock<lock::LWLatch> LockPartition(const TKey &key) {
        auto index = this->getElementIndex(key);
        return std::unique_lock{this->_maps[index].Latch};
    }

    std::shared_lock<lock::LWLatch> LockPartitionShared(const TKey &key) {
        auto index = this->getElementIndex(key);
        return std::shared_lock{this->_maps[index].Latch};
    }

    // Set value with given key.
    // Returns pair of value pointer and bool - whether insert succeeded.
    // If true then we have inserted the entry. Otherwise entry with given key
    // already exist and value pointer is an existing entry.
    std::pair<TValue *, bool> Insert(const TKey &key, const TValue &value) {
        auto index = this->getElementIndex(key);
        return this->_maps[index].Map.insert(key, value);
    }

    TValue *Get(const TKey &key) {
        auto index = this->getElementIndex(key);
        auto map = &this->_maps[index].Map;
        auto it = map->find(key);
        if (it == map->end()) {
            return nullptr;
        } else {
            return &it->second;
        }
    }

    bool Remove(const TKey &key) {
        auto index = this->getElementIndex(key);
        auto map = &this->_maps[index].Map;
        auto erased = map->erase(key);
        assert(erased <= 1);
        return erased == 1;
    }

    bool Has(const TKey &key) {
        auto index = this->getElementIndex(key);
        auto map = &this->_maps[index].Map;
        return map->contains(key);
    }

    HashTable(HashTable &&other) = delete;
    HashTable(const HashTable &other) = delete;
    HashTable &operator=(HashTable &&other) = delete;
    HashTable &operator=(const HashTable &other) = delete;
};
}; // namespace mi::adt
