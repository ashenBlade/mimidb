#pragma once

#include <memory>

#include "access/table/ITable.hpp"
#include "access/table/Oid.hpp"
#include "adt/HashTable.hpp"
#include "db/Schema.hpp"
#include "utils/NonCopyable.hpp"

namespace mi::db {
class Database {
  private:
    // Mapping from table's OID to it's object
    std::unique_ptr<Schema> _schema;

  public:
    // Constructor for default database - now only it exists
    explicit Database(std::unique_ptr<Schema> schema);
    Database(Database &&) = default;
    Database &operator=(Database &&) = default;

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Open user table with given table id
    std::shared_ptr<mi::access::table::ITable> OpenTable(mi::Oid tableId);

    /// @brief Get database schema
    Schema *GetSchema() {
      return this->_schema.get();
    }
    const Schema *GetSchema() const {
      return this->_schema.get();
    }
};
}; // namespace mi::db
