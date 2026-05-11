#pragma once

#include <string>

#include "executor/Datum.hpp"

namespace mi::db::builtin {

// int32
std::string Int64Output(Datum value);
std::string Int32Output(Datum value);
std::string Int16Output(Datum value);

} // namespace mi::db::builtin
