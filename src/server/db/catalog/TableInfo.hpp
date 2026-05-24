#pragma once

#include "access/table/TupleDescriptor.hpp"
#include "db/catalog/ColumnInfo.hpp"
#include "executor/Oid.hpp"
#include <assert.h>
#include <vector>

namespace mi::db::catalog {
class TableInfo {
  private:
    /// @brief Table Oid
    Oid _oid;
    // Tuple descriptor
    const access::table::TupleDescriptor *_descriptor;
    // All columns in table
    std::vector<ColumnInfo> _columns;

  public:
    TableInfo(Oid oid, const access::table::TupleDescriptor *descriptor, std::vector<ColumnInfo> columns)
        : _oid(oid), _descriptor(descriptor), _columns(std::move(columns)) {
        assert(this->_descriptor->GetMaxAttrNumber() == this->_columns.size());
    };

    Oid Id() const { return this->_oid; }
    const access::table::TupleDescriptor *GetDescriptor() const { return this->_descriptor; }
    const std::vector<ColumnInfo> &GetColumns() const { return this->_columns; }
    const ColumnInfo *FindColumn(const std::string &colname) const {
        for (const auto &col : this->_columns) {
            if (col.GetName() == colname) {
                return &col;
            }
        }

        return nullptr;
    }
};
} // namespace mi::db::catalog
