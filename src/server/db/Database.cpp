#include "db/Database.hpp"
#include "access/heap/HeapTable.hpp"
#include "db/Schema.hpp"
#include <memory>

using namespace mi::db;

Database::Database(std::unique_ptr<Schema> schema) : _schema(std::move(schema)) {}

std::shared_ptr<mi::access::table::ITable> Database::OpenTable(mi::Oid relid) {
    assert(relid.IsValid());

    auto &tableInfo = this->_schema->GetTableInfo(relid);
    return std::make_unique<access::heap::HeapTable>(relid, tableInfo.GetDescriptor());
}