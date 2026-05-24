#pragma once

#include <cstdint>

namespace mi::storage::trans {

enum ResourceManagerId : int32_t {
    Invalid = 0,
    Heap = 1,
};

};