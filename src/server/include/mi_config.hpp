#pragma once

namespace mi {
// Global configuration for whole cluster
class Config final {
  public:
    // Size of table page, 8KB
    static constexpr unsigned int PageSize = 8192;
    // Number of bits per byte
    static constexpr unsigned int BitsPerByte = 8;
    // Max workers per cluster
    static constexpr unsigned int MaxWorkers = 16;
    // Socket port for listening connections
    static constexpr unsigned int Port = 6543;
};
} // namespace mi
