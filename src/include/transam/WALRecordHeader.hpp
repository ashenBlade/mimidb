#pragma once

#include "transam/ResourceManagerId.hpp"
#include "transam/TransactionId.hpp"
namespace mi::transam {

struct WALRecordHeader {
    /// TransactionId created this record
    TransactionId Xid;
    /// Resource manager for this record
    ResourceManagerId RMgrId;
    /// Length of data
    size_t Length;
};

};
