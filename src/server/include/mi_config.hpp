#pragma once

namespace mi {
// Global configuration for whole cluster
class Config final {
  public:
    // Size of table page, 8KB
    static constexpr unsigned int PageSize = 8192;
    // Number of bits per byte
    static constexpr unsigned int BitsPerByte = 8;
};
} // namespace mi
