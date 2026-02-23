#include "mimidb.hpp"

#include "access/heap/HeapTable.hpp"

#include <stdexcept>



void mi::access::heap::HeapTable::InsertTuple(std::shared_ptr<ITuple> tuple) {
    throw std::runtime_error("not implemented");
}

void mi::access::heap::HeapTable::UpdateTuple(std::shared_ptr<ITuple> oldTuple, std::shared_ptr<ITuple> newTuple) {
    throw std::runtime_error("not implemented");
}

void mi::access::heap::HeapTable::DeleteTuple(std::shared_ptr<ITuple> tuple) {
    throw std::runtime_error("not implemented");
}
