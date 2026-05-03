#pragma once

#include "access/table/AttrNumber.hpp"
#include "access/table/Datum.hpp"
#include "access/table/ITuple.hpp"
#include <optional>
namespace mi::executor {
class VirtualTuple : public access::table::ITuple {
  private:
    std::vector<Datum> _values;
    std::vector<bool> _isnull;

  public:
    VirtualTuple(std::vector<Datum> &&values, std::vector<bool> &&isnull)
        : _values(std::move(values)), _isnull(std::move(isnull)) {};
    std::optional<Datum> GetAttribute(access::table::AttrNumber attno) override {
        if (this->_isnull[attno.ToIndex()]) {
            return std::nullopt;
        } else {
            return this->_values[attno.ToIndex()];
        }
    }
    ~VirtualTuple() override {};
};
} // namespace mi::executor
