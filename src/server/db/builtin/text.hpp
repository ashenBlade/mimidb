#pragma once

#include <memory>
#include <optional>
#include <string>
#include <cstdint>

#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"

namespace mi::db::builtin {

struct Text {
    // Length of string
    uint32_t Length;

    // Actual string follows this structure:

    const char *GetCString() const {
        return reinterpret_cast<const char *>(this) + sizeof(uint32_t);
    }

    static std::unique_ptr<Text> FromCString(const char *str);
};

// Output functions
std::string TextOutput(Datum value);

// Equality operators
std::optional<Datum> TextEq(executor::FunctionArgs &args);

}
