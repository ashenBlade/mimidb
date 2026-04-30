#pragma once

#include "db/catalog/TableInfo.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "utils/NonCopyable.hpp"

namespace mi::db {
class Schema : private NonCopyable {
  private:
    /// @brief Table ID to table info
    std::unordered_map<Oid, catalog::TableInfo> _tables;
    /// @brief Type ID to type information
    std::unordered_map<Oid, catalog::TypeInfo> _types;
  public:
    Schema(std::unordered_map<Oid, catalog::TableInfo> &&tables, std::unordered_map<Oid, catalog::TypeInfo> &&types);

    /// @brief Get information about table by it's Id
    const catalog::TableInfo &GetTableInfo(Oid tableId) const;
    /// @brief Get information about type by it's Id
    const catalog::TypeInfo & GetTypeInfo(Oid typeId) const;
};
} // namespace mi::db
