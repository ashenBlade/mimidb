#pragma once

#include "access/table/AttrNumber.hpp"
#include "executor/Oid.hpp"
#include <string>
namespace mi::db::catalog {
class ColumnInfo {
  private:
    Oid _typeId;
    std::string _name;
    access::table::AttrNumber _attno;

  public:
    ColumnInfo(Oid typeId, std::string name, access::table::AttrNumber attno) : _typeId(typeId), _name(std::move(name)), _attno(attno) {};
    Oid GetId() const { return this->_typeId; }
    access::table::AttrNumber AttrNumber() const { return this->_attno; };
    const std::string &GetName() const { return this->_name; };
};
} // namespace mi::db::catalog