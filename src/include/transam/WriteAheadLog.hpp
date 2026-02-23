#pragma once

#include "transam/LogSeqNumber.hpp"

namespace mi::transam {

class WriteAheadLog {
private:
    /// @brief File descriptor for underlying file
    int _fd;
public:
    /// @brief Durable write record to WAL
    /// @return LSN at which record is written
    LogSeqNumber WriteLogRecord();
};

};
