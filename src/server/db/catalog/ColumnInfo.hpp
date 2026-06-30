#pragma once

#include "access/AttrNumber.hpp"
#include "executor/Oid.hpp"
#include <string>
namespace mi::db::catalog {
class ColumnInfo {
  private:
    Oid _typeId;
    std::string _name;
    access::AttrNumber _attno;

  public:
    ColumnInfo(Oid typeId, std::string name, access::AttrNumber attno) : _typeId(typeId), _name(std::move(name)), _attno(attno) {};
    Oid GetId() const { return this->_typeId; }
    access::AttrNumber AttrNumber() const { return this->_attno; };
    const std::string &GetName() const { return this->_name; };
};
} // namespace mi::db::catalog