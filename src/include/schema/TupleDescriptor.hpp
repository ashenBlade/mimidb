#pragma once

#include <vector>

#include "schema/AttrNumber.hpp"
#include "schema/Oid.hpp"

namespace mi::schema {

class TupleDescriptor {
  private:
    /// @brief Maximal attribute number in tuple
    AttrNumber _maxAttrNumber;
    /// @brief Types for each attribute. Attribute number is 1-based, so
    std::vector<Oid> _attributesTypes;
  public:
    Oid GetAttributeType(AttrNumber attno) const;
    AttrNumber GetNumberOfAttributes() const;
};

}; // namespace mi::executor