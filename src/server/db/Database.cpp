#include "db/Database.hpp"
#include "db/Schema.hpp"
#include <memory>

using namespace mi::db;

Database::Database(std::unique_ptr<Schema> schema,
                   std::unordered_map<Oid, std::unique_ptr<access::ITable>> relations)
    : _schema(std::move(schema)), _relations(std::move(relations)) {}

mi::access::ITable *Database::OpenTable(mi::Oid relid) {
    assert(relid.IsValid());
    auto it = this->_relations.find(relid);
    return it->second.get();
}