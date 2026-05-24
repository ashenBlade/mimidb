#pragma once

namespace mi {
// Special class to inherit and forbid copying and moving
class NonCopyable {
  public:
    NonCopyable() = default;

    NonCopyable(const NonCopyable &other) = delete;
    NonCopyable(NonCopyable &&other) = delete;
    NonCopyable &operator=(const NonCopyable &other) = delete;
    NonCopyable &operator=(NonCopyable &&other) = delete;
};
}; // namespace mi
