#pragma once

#include "catalog/OperatorInfo.hpp"
#include "db/catalog/OperatorInfo.hpp"
#include "db/catalog/TableInfo.hpp"
#include "db/catalog/TypeInfo.hpp"
#include "executor/Oid.hpp"
#include "utils/NonCopyable.hpp"
#include <memory>
#include <unordered_map>

// Hash functions for schema indexes, etc..
namespace std {
template <> struct hash<std::tuple<mi::Oid, mi::Oid, mi::db::catalog::OperatorStrategy>> {
    size_t
    operator()(const std::tuple<mi::Oid, mi::Oid, mi::db::catalog::OperatorStrategy> &tuple) const {
        return std::hash<mi::Oid>{}(std::get<0>(tuple)) ^ std::hash<mi::Oid>{}(std::get<1>(tuple)) ^
               std::hash<mi::db::catalog::OperatorStrategy>{}(std::get<2>(tuple));
    }
};
} // namespace std

namespace mi::db {
class Schema : private NonCopyable {
  private:
    /// @brief Table ID to table info
    std::unordered_map<Oid, std::unique_ptr<catalog::TableInfo>> _tables;
    /// @brief Type ID to type information
    std::unordered_map<Oid, std::unique_ptr<catalog::TypeInfo>> _types;
    /// @brief Operator ID to operator information
    std::unordered_map<Oid, std::unique_ptr<catalog::OperatorInfo>> _operators;
    // Index to find Operator Info by it's signature
    std::unordered_map<std::tuple<Oid, Oid, catalog::OperatorStrategy>, catalog::OperatorInfo *>
        _signatureToOperatorIdx;

  public:
    Schema(std::unordered_map<Oid, std::unique_ptr<catalog::TableInfo>> &&tables,
           std::unordered_map<Oid, std::unique_ptr<catalog::TypeInfo>> &&types,
           std::unordered_map<Oid, std::unique_ptr<catalog::OperatorInfo>> &&operators);

    /// @brief Get information about table by it's Id
    const catalog::TableInfo &GetTableInfo(Oid tableId) const;
    /// @brief Get information about type by it's Id
    const catalog::TypeInfo &GetTypeInfo(Oid typeId) const;
    /// @brief Get information about operator by it's Id
    const catalog::OperatorInfo &GetOperatorInfo(Oid operatorId) const;
    /// @brief Search operator by it's signature
    const catalog::OperatorInfo *FindOperatorInfo(Oid leftType, Oid rightType,
                                                  catalog::OperatorStrategy strategy) const;
};
} // namespace mi::db
