#pragma once

#include <vector>

#include "access/table/AttrNumber.hpp"
#include "access/table/Oid.hpp"

namespace mi::access::table {
class AttributeDescriptor {
  private:
    // Type of this attribute
    Oid _typeId;
    // Length of attribute or -1 if variable width
    int16_t _length;
    // If true this is value type, or reference (variable width) type
    bool _byVal;

  public:
    AttributeDescriptor(Oid typeId, int16_t length, bool byVal)
        : _typeId(typeId), _length(length), _byVal(byVal) {};

    Oid TypeId() const { return this->_typeId; }
    int16_t Length() const { return this->_length; }
    bool ByVal() const { return this->_byVal; }
};

class TupleDescriptor {
  private:
    /// Array of attribute descriptors
    std::vector<AttributeDescriptor> _attributes;

  public:
    TupleDescriptor(const std::vector<AttributeDescriptor> &attributes) : _attributes(attributes) {};
    TupleDescriptor(std::vector<AttributeDescriptor> &&attributes)
        : _attributes(std::move(attributes)) {};
    std::vector<AttributeDescriptor> &Attributes() { return this->_attributes; }
    const std::vector<AttributeDescriptor> &Attributes() const { return this->_attributes; }
    AttrNumber GetMaxAttrNumber() const {
      return AttrNumber{static_cast<AttrNumber::type>(this->_attributes.size())};
    }
};

}; // namespace mi::access::table