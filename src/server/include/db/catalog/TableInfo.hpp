#pragma once

#include "access/table/Oid.hpp"
#include "access/table/TupleDescriptor.hpp"

namespace mi::db::catalog {
class TableInfo {
  private:
    /// @brief Table Oid
    Oid _oid;
    access::table::TupleDescriptor _descriptor;

  public:
    TableInfo(Oid oid, access::table::TupleDescriptor descriptor)
        : _oid(oid), _descriptor(descriptor) {};

    Oid Id() const { return this->_oid; }
    const access::table::TupleDescriptor *GetDescriptor() const { return &this->_descriptor; }
};
} // namespace mi::db::catalog
