#include <iostream>

#include "mimidb.hpp"

#include "executor/Datum.hpp"
#include "transam/TransactionId.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    std::cout << "page size: " << mi::PAGESIZE << std::endl;
    void *pointer = NULL;
    auto datum = mi::executor::Datum {pointer};

    std::cout << datum.getInt<int>() << std::endl;
    return 0;
}
