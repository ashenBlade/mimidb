#pragma once

#include "access/AttrNumber.hpp"
#include "access/ITuple.hpp"
#include "executor/Datum.hpp"
#include <optional>
#include <vector>

namespace mi::executor {
class VirtualTuple : public access::ITuple {
  private:
    std::vector<Datum> _values;
    std::vector<bool> _isnull;

  public:
    VirtualTuple(std::vector<Datum> &&values, std::vector<bool> &&isnull)
        : _values(std::move(values)), _isnull(std::move(isnull)) {};
    std::optional<Datum> GetAttribute(access::AttrNumber attno) override {
        if (this->_isnull[attno.ToIndex()]) {
            return std::nullopt;
        } else {
            return this->_values[attno.ToIndex()];
        }
    }
    void SetAttribute(access::AttrNumber attno, std::optional<Datum> value) {
        if (value.has_value()) {
            this->_values[attno.ToIndex()] = value.value();
            this->_isnull[attno.ToIndex()] = false;
        } else {
            this->_values[attno.ToIndex()] = Datum{};
            this->_isnull[attno.ToIndex()] = true;
        }
    }
    access::AttrNumber GetMaxAttno() override {
        return access::AttrNumber{static_cast<uint16_t>(this->_values.size())};
    }
    ~VirtualTuple() override = default;

    static VirtualTuple Copy(access::ITuple &tuple) {
        auto natts = tuple.GetMaxAttno();
        auto values = std::vector<Datum>(natts);
        auto isnull = std::vector<bool>(natts);
        for (auto attno = access::AttrNumber::Min(); attno <= natts; ++attno) {
            auto attr = tuple.GetAttribute(attno);
            if (attr.has_value()) {
                values[attno.ToIndex()] = attr.value();
                isnull[attno.ToIndex()] = false;
            } else {
                values[attno.ToIndex()] = Datum{0};
                isnull[attno.ToIndex()] = true;
            }
        }

        return VirtualTuple{std::move(values), std::move(isnull)};
    }
};
} // namespace mi::executor
