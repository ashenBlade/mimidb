#pragma once

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"
#include <cstddef>

namespace mi::storage::wal {

struct WALRecordHeader {
    /// TransactionId created this record
    trans::TransactionId Xid;
    /// Resource manager for this record
    trans::ResourceManagerId RMgrId;
    /// Length of data
    size_t Length;
};

}; // namespace mi::transam
