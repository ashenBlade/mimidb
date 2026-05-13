#include "db/bulitin/int.hpp"
#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"
#include <cassert>
#include <optional>

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

template <class T> std::optional<mi::Datum> inteqgeneric(mi::executor::FunctionArgs &args) {
    assert(args.GetNArgs() == 2);

    auto first = args.GetArg(0);
    auto second = args.GetArg(1);
    return mi::Datum{first->getScalar<T>() == second->getScalar<T>()};
}

std::optional<mi::Datum> mi::db::builtin::Int64Eq(executor::FunctionArgs &args) {
    return inteqgeneric<int64_t>(args);
};

std::optional<mi::Datum> mi::db::builtin::Int32Eq(executor::FunctionArgs &args) {
    return inteqgeneric<int32_t>(args);
};

std::optional<mi::Datum> mi::db::builtin::Int16Eq(executor::FunctionArgs &args) {
    return inteqgeneric<int16_t>(args);
};

template <class T> std::optional<mi::Datum> intaddgeneric(mi::executor::FunctionArgs &args) {
    assert(args.GetNArgs() == 2);

    auto first = args.GetArg(0);
    auto second = args.GetArg(1);
    return mi::Datum{first->getScalar<T>() + second->getScalar<T>()};
}

std::optional<mi::Datum> mi::db::builtin::Int64Add(mi::executor::FunctionArgs &args) {
    return intaddgeneric<int64_t>(args);
}

std::optional<mi::Datum> mi::db::builtin::Int32Add(mi::executor::FunctionArgs &args) {
    return intaddgeneric<int32_t>(args);
}

std::optional<mi::Datum> mi::db::builtin::Int16Add(mi::executor::FunctionArgs &args) {
    return intaddgeneric<int16_t>(args);
}
