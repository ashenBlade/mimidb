#pragma once

#include "executor/Datum.hpp"
#include "utils/DatumArray.hpp"
#include <optional>
namespace mi::executor {
class FunctionArgs {
  private:
    DatumArray _args;

  public:
    FunctionArgs(DatumArray args) : _args(std::move(args)) {};

    std::optional<Datum> GetArg(size_t index) { return this->_args.Get(index); }
    size_t GetNArgs() const { return this->_args.Size(); }
};
} // namespace mi::executor
