#include "db/bulitin/int.hpp"

using namespace mi::db::builtin;

std::string mi::db::builtin::Int16Output(Datum value) {
    return std::to_string(value.getScalar<int16_t>());
}

std::string mi::db::builtin::Int32Output(Datum value) {
    return std::to_string(value.getScalar<int32_t>());
}

std::string mi::db::builtin::Int64Output(Datum value) {
    return std::to_string(value.getScalar<int64_t>());
}
