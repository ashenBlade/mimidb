#include "packets/CommandCompletePacket.hpp"
#include "packets/IPacketVisitor.hpp"

using namespace mi::interface::libmimi;

const std::string &CommandCompletePacket::GetTag() const {
    return this->_tag;
}

int64_t CommandCompletePacket::GetRowsCount() const {
    return this->_rows;
}

void CommandCompletePacket::Accept(IPacketVisitor &visitor) const {
    visitor.Visit(*this);
}
