#pragma once

namespace mi::lock {
class Spin {
  public:
    /// @brief Perform basic spin
    static void PerformSpin();
};
} // namespace mi::lock