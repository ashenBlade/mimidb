#pragma once

#include "executor/func/FunctionArgs.hpp"
#include "executor/func/MIFunction.hpp"
#include <optional>

namespace mi::executor {
class FunctionContext {
  private:
    // Function to invoke
    MIFunction _function;
    // Function is strict
    bool _strict;
    // Amount of arguments for function to pass
    size_t _nargs;

  public:
    FunctionContext(MIFunction function, bool strict, size_t nargs)
        : _function(function), _strict(strict), _nargs(nargs) {};
    bool IsStrict() const { return this->_strict; }
    size_t GetNArgs() const { return this->_nargs; }
    std::optional<Datum> Invoke(FunctionArgs &args) { return this->_function(args); }
    std::optional<Datum> Invoke(FunctionArgs &&args) { return this->_function(args); }
};
} // namespace mi::executor
