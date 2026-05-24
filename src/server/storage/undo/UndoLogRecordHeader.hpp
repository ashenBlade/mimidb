#pragma once

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"
#include <cstdint>

namespace mi::storage::undo {

struct UndoLogRecordHeader {
    /// TransactionId created this record
    trans::TransactionId Xid;
    // Id of resource manager for this record
    trans::ResourceManagerId ResourceManager;
    // Resource manager specific type of record
    uint8_t RecordType;
    // Length of data
    uint64_t DataLength;
};

} // namespace mi::transam
