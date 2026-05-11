#include "db/Schema.hpp"
#include "db/catalog/TableInfo.hpp"
#include "executor/Oid.hpp"
#include <stdexcept>

using namespace mi::db;

Schema::Schema(std::unordered_map<Oid, catalog::TableInfo> &&tables,
               std::unordered_map<Oid, catalog::TypeInfo> &&types)
    : _tables(std::move(tables)), _types(std::move(types)) {};

template <class T>
static const T &map_get_entry(mi::Oid id, const std::unordered_map<mi::Oid, T> &map) {
    auto it = map.find(id);
    if (it != map.end()) {
        return it->second;
    }

    throw std::runtime_error("could not find entry with provided id");
}

const catalog::TableInfo &Schema::GetTableInfo(Oid tableId) const {
    return map_get_entry(tableId, this->_tables);
}

const catalog::TypeInfo &Schema::GetTypeInfo(Oid typeId) const {
    return map_get_entry(typeId, this->_types);
}
