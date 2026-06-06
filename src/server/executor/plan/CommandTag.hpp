#pragma once

namespace mi::executor::plan {

// Represents different executable statements in system
enum class CommandTag {
    Select,
    Update,
    Insert,
    Delete,
};

}
