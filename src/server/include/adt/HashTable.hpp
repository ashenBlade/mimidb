#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace mi::adt {
template <class TKey, class TValue, class THash = std::hash<TKey>,
          class TKeyEqual = std::equal_to<TKey>>
class HashTable {
  private:
    /// @brief lock to protect contents during operations
    std::shared_mutex _lock;
    /// @brief actual hash table data
    std::unordered_map<TKey, TValue, THash, TKeyEqual> _hash;

  public:
    HashTable() : _lock(), _hash() {};

    void Set(const TKey &key, const TValue &value) {
        auto guard = std::scoped_lock{this->_lock};
        this->_hash[key] = value;
    }

    TValue *Get(const TKey &key) {
        auto guard = std::shared_lock{this->_lock};
        if (auto entry = this->_hash.find(key); entry != this->_hash.end()) {
            return &entry->second;
        } else {
            return nullptr;
        }
    }

    bool Remove(const TKey &key) {
        auto guard = std::scoped_lock{this->_lock};
        auto result = this->_hash.erase(key);
        return result > 0;
    }

    bool Has(const TKey &key) {
        auto guard = std::shared_lock{this->_lock};
        return this->_hash.count(key) > 0;
    }

    HashTable(HashTable &&other) = delete;
    HashTable(const HashTable &other) = delete;
    HashTable &operator=(HashTable &&other) = delete;
    HashTable &operator=(const HashTable &other) = delete;
};
}; // namespace mi::adt
