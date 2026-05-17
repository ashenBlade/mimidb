#include "packets/QueryPacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

void QueryPacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
