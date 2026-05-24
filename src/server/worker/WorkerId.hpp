#pragma once

#include <cstdint>
namespace mi::worker {
struct WorkerId final {
    // actual value of worker id
    int32_t value;

    WorkerId() : value(WorkerId::InvalidValue) {}
    WorkerId(int32_t value) : value(value) {}
    operator int32_t() { return value; }
    operator std::size_t() {
        // WorkerId sometimes is used as index in arrays, so add size_t cast
        // to be used in operator[]
        return static_cast<std::size_t>(this->value);
    }
    
    bool IsValid() const { return value != WorkerId::InvalidValue; };
    bool operator==(const WorkerId &other) const { return this->value == other.value; };
    bool operator!=(const WorkerId &other) const { return !(*this == other); };

    static const int32_t InvalidValue = -1;
    static WorkerId Invalid() {
        return WorkerId{InvalidValue};
    }
};
}; // namespace mi::worker