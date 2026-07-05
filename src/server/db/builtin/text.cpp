#include "text.hpp"

#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

using namespace mi::db::builtin;

std::unique_ptr<Text> Text::FromCString(const char *str) {
    auto length = std::strlen(str);
    if (length > UINT32_MAX) {
        throw std::runtime_error("too large c string");
    }

    auto buffer = std::make_unique<char[]>(sizeof(uint32_t) + length + 1);
    auto obj = reinterpret_cast<Text *>(buffer.get());

    obj->Length = static_cast<uint32_t>(length);

    auto s = const_cast<char *>(obj->GetCString());
    std::strncpy(s, str, length);

    return std::unique_ptr<Text>{reinterpret_cast<Text *>(buffer.release())};
}

std::optional<mi::Datum> mi::db::builtin::TextEq(executor::FunctionArgs &args) {
    assert(args.GetNArgs() == 2);

    auto first = args.GetArg(0)->getPointer<Text>();
    auto second = args.GetArg(1)->getPointer<Text>();

    if (first->Length == second->Length && strncmp(first->GetCString(), second->GetCString(), first->Length) == 0) {
        return Datum{true};
    } else {
        return Datum{false};
    }
}

// Output functions
std::string mi::db::builtin::TextOutput(Datum value) {
    auto text= value.getPointer<Text>();
    return std::string{text->GetCString(), text->Length};
}