#pragma once

#include "executor/Datum.hpp"
#include <optional>
#include <stdexcept>
#include <vector>
namespace mi {
class DatumArray {
  private:
    // Actual values
    std::vector<Datum> _values;
    // NULL flags
    std::vector<bool> _isnull;
    // Amount of entries in arrays
    size_t _size;

  public:
    DatumArray() : _values(), _isnull(), _size(0) {};
    DatumArray(size_t size) : _values(size), _isnull(size), _size(0) {};
    DatumArray(std::vector<Datum> values, std::vector<bool> isnull)
        : _values(std::move(values)), _isnull(std::move(isnull)) {
        if (this->_values.size() != this->_isnull.size()) {
            throw std::runtime_error("datum array and isnull bitmap have different sizes");
        }

        _size = this->_values.size();
    };

    DatumArray(const DatumArray &other) = default;
    DatumArray &operator=(const DatumArray &other) = default;

    DatumArray(DatumArray &&other) = default;
    DatumArray &operator=(DatumArray &&other) = default;

    void AddValue(Datum value) {
        this->_values.push_back(value);
        this->_isnull.push_back(false);
        _size++;
    }

    void AddNull() {
        this->_values.push_back(Datum{0});
        this->_isnull.push_back(true);
        _size++;
    }

    void SetValue(size_t index, Datum value) {
        this->_values[index] = value;
        this->_isnull[index] = false;
    }

    void SetNull(size_t index) {
        this->_values[index] = Datum{0};
        this->_isnull[index] = true;
    }

    void Set(size_t index, std::optional<Datum> value) {
        if (value.has_value()) {
            this->_values[index] = value.value();
            this->_isnull[index] = false;
        } else {
            this->_values[index] = Datum{0};
            this->_isnull[index] = true;
        }
    }
    std::optional<Datum> Get(size_t index) const {
        std::optional<Datum> result;
        if (this->_isnull[index]) {
            result = std::nullopt;
        } else {
            result = this->_values[index];
        }

        return result;
    }

    size_t Size() const { return this->_values.size(); };
};
} // namespace mi
