#include "packets/CommandCompletePacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

void CommandCompletePacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
