#include "mimidb.hpp"

#include "access/heap/HeapTableScan.hpp"

#include <stdexcept>

using namespace mi::access::heap;

void HeapTableScan::BeginScan() {
    // Nothing
};

std::unique_ptr<mi::executor::ITuple> mi::access::heap::HeapTableScan::GetNextTuple() {
    throw std::runtime_error("not implemented");
}

void mi::access::heap::HeapTableScan::EndScan() {
    throw std::runtime_error("not implemented");
}
