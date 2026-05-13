#pragma once

#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"
#include <functional>
#include <optional>
namespace mi::executor {
    // std::optional<Datum> SampleFunction(FunctionArgs &args);
    using MIFunction = std::function<std::optional<Datum>(FunctionArgs &)>;
}