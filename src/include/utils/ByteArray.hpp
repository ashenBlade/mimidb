#pragma once

#include <string>

namespace mi {

class ByteArray : public std::basic_string<std::byte> {
  public:
    const char *c_arr() const { return reinterpret_cast<const char *>(this->c_str()); }
    
};
} // namespace mi
