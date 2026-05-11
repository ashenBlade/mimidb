#pragma once

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"
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
