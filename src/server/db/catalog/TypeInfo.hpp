#pragma once

#include "executor/Datum.hpp"
#include "executor/Oid.hpp"
#include <functional>
#include <string>

namespace mi::db::catalog {
class TypeInfo {
  public:
    using OutputFunction = std::function<std::string(Datum)>;

  private:
    /// @brief Global id for this type
    Oid _id;
    /// @brief Byte size of type or -1 if variable width
    int _size;
    /// @brief Serialization function
    OutputFunction _output;

  public:
    TypeInfo(Oid id, int size, OutputFunction output) : _id(id), _size(size), _output(output) {};

    Oid Id() const { return this->_id; };
    int Size() const { return this->_size; };
    OutputFunction GetOutputFunction() const { return this->_output; };
};
} // namespace mi::db::catalog
