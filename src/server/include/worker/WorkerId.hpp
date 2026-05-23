#pragma once

namespace mi::worker {
struct WorkerId final {
    // actual value of worker id
    int value;

    WorkerId() : value(WorkerId::InvalidValue) {}
    WorkerId(int value) : value(value) {}
    operator int() { return value; }
    
    bool IsValid() const { return value != WorkerId::InvalidValue; };
    bool operator==(const WorkerId &other) { return this->value == other.value; };
    bool operator!=(const WorkerId &other) { return !(*this == other); };
    
    static const int InvalidValue = -1;
    static WorkerId Invalid() {
        return WorkerId{InvalidValue};
    }
};
}; // namespace mi::worker