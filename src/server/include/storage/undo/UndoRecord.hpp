#pragma once

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"
#include <vector>

namespace mi::storage::undo {
class UndoRecord {
  public:
    trans::TransactionId Xid;
    trans::ResourceManagerId RMgrId;
    uint8_t Type;
    std::vector<std::byte> Payload;

    UndoRecord(trans::TransactionId xid, trans::ResourceManagerId rmgrid, uint8_t type,
               std::vector<std::byte> payload)
        : Xid(xid), RMgrId(rmgrid), Type(type), Payload(std::move(payload)) {};

    template <class T> T *GetPayload() { return reinterpret_cast<T *>(this->Payload.data()); }
};
} // namespace mi::transam
