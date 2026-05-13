#include "db/bulitin/intop.hpp"
#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"
#include <assert.h>
#include <optional>

using namespace mi::db::builtin;

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