#pragma once

#include "executor/func/FunctionArgs.hpp"
#include <optional>

namespace mi::db::builtin {

std::optional<Datum> Int64Eq(executor::FunctionArgs &args);
std::optional<Datum> Int32Eq(executor::FunctionArgs &args);
std::optional<Datum> Int16Eq(executor::FunctionArgs &args);

}
