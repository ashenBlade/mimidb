#pragma once

#include "access/table/ITable.hpp"
#include "access/table/Oid.hpp"
#include "adt/HashTable.hpp"
#include "transam/UndoLog.hpp"
#include "transam/WriteAheadLog.hpp"

namespace mi::db {
class Database {
  private:
    // WAL for this database
    mi::transam::WriteAheadLog _wal;
    // Undo log for database
    mi::transam::UndoLog _undo;
    // Mapping from table's OID to it's object
    mi::adt::HashTable<mi::access::table::Oid, mi::access::table::ITable> _schema;

  public:
    // Constructor for default database - now only it exists
    Database();

    // Get UNDO log for this database
    mi::transam::UndoLog &GetUndoLog();
    // Get WAL for this database
    mi::transam::WriteAheadLog &GetWAL();
    // Open user table with given table id
    mi::access::table::ITable *OpenTable(mi::access::table::Oid tableId);
};
}; // namespace mi::db
