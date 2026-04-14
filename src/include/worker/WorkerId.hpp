#pragma once

namespace mi::worker {
struct WorkerId final {
    int value;

    WorkerId() : value(WorkerId::Invalid) {}
    WorkerId(int value) : value(value) {}
    operator int() { return value; }

    bool IsValid() const { return value != WorkerId::Invalid; };

    static constexpr const int Invalid = -1;
};
}; // namespace mi::worker