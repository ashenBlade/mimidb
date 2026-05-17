#include "packets/ErrorResponsePacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

void ErrorResponsePacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
