#pragma once

#include "executor/ITuple.hpp"

namespace mi::executor::receiver {
class ITupleReceiver {
  public:
    virtual void PutTuple(mi::executor::ITuple &tuple) = 0;
};
}; // namespace mi::executor::command