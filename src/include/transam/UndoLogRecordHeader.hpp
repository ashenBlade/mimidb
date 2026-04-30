#pragma once

#include <cstdint>

#include "transam/ResourceManagerId.hpp"

namespace mi::transam {

struct UndoLogRecordHeader {
    // Length of data
    uint32_t DataLength;
    // Id of resource manager for this record
    ResourceManagerId ResourceManager;
};

}
