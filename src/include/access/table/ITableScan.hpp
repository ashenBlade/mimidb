#pragma once

#include <memory>

#include "executor/ITuple.hpp"

namespace mi::access::table {

/// @brief Access method specific table iterator state
class ITableScan {
public:
    /// @brief Start iteration and prepare state
    virtual void BeginScan() = 0;
    /// @brief Get next tuple from underlying table
    /// @return Tuple or NULL if end of scan
    virtual std::unique_ptr<mi::executor::ITuple> GetNextTuple() = 0;
    /// @brief End iteration and cleanup resources
    virtual void EndScan() = 0;

    virtual ~ITableScan() = 0;
};

};
