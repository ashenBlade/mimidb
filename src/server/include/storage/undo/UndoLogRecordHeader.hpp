#pragma once

#include <cstdint>

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"

namespace mi::transam {

struct UndoLogRecordHeader {
    /// TransactionId created this record
    TransactionId Xid;
    // Id of resource manager for this record
    ResourceManagerId ResourceManager;
    // Resource manager specific type of record
    uint8_t RecordType;
    // Length of data
    uint64_t DataLength;
};

}
