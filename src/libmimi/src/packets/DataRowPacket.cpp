#include "packets/DataRowPacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

void DataRowPacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
