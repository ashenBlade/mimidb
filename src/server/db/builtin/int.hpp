#pragma once

#include <optional>
#include <string>

#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"

namespace mi::db::builtin {

// Output functions
std::string Int64Output(Datum value);
std::string Int32Output(Datum value);
std::string Int16Output(Datum value);

// Equality operators
std::optional<Datum> Int64Eq(executor::FunctionArgs &args);
std::optional<Datum> Int32Eq(executor::FunctionArgs &args);
std::optional<Datum> Int16Eq(executor::FunctionArgs &args);

// Sum operators
std::optional<Datum> Int64Add(executor::FunctionArgs &args);
std::optional<Datum> Int32Add(executor::FunctionArgs &args);
std::optional<Datum> Int16Add(executor::FunctionArgs &args);

} // namespace mi::db::builtin
