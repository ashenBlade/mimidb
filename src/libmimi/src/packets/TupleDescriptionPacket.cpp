#include "packets/TupleDescriptionPacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

void TupleDescriptionPacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
