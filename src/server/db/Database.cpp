#include "db/Database.hpp"
#include "access/heap/HeapTable.hpp"
#include "db/Schema.hpp"
#include <memory>

using namespace mi::db;

Database::Database(std::unique_ptr<Schema> schema,
                   std::unordered_map<Oid, std::unique_ptr<access::table::ITable>> relations)
    : _schema(std::move(schema)), _relations(std::move(relations)) {}

mi::access::table::ITable *Database::OpenTable(mi::Oid relid) {
    assert(relid.IsValid());
    auto it = this->_relations.find(relid);
    return it->second.get();
}