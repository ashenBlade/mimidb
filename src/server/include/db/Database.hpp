#pragma once

#include "access/table/ITable.hpp"
#include "db/Schema.hpp"
#include "executor/Oid.hpp"
#include "utils/NonCopyable.hpp"
#include <memory>
#include <unordered_map>

namespace mi::db {
class Database : private NonCopyable {
  private:
    // Mapping from table's OID to it's object
    std::unique_ptr<Schema> _schema;
    // All relations in database
    std::unordered_map<Oid, std::unique_ptr<access::table::ITable>> _relations;

  public:
    // Constructor for default database - now only it exists
    explicit Database(std::unique_ptr<Schema> schema, std::unordered_map<Oid, std::unique_ptr<access::table::ITable>> relations);

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Open user table with given table id
    mi::access::table::ITable *OpenTable(mi::Oid tableId);

    /// @brief Get database schema
    Schema *GetSchema() { return this->_schema.get(); }
    const Schema *GetSchema() const { return this->_schema.get(); }
};
}; // namespace mi::db
