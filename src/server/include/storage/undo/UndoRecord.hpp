#pragma once

#include "trans/ResourceManagerId.hpp"
#include "trans/TransactionId.hpp"
#include <vector>

namespace mi::transam {
class UndoRecord {
  public:
    TransactionId Xid;
    ResourceManagerId RMgrId;
    uint8_t Type;
    std::vector<std::byte> Payload;

    UndoRecord(TransactionId xid, ResourceManagerId rmgrid, uint8_t type,
               std::vector<std::byte> payload)
        : Xid(xid), RMgrId(rmgrid), Type(type), Payload(std::move(payload)) {};

    template <class T> T *GetPayload() { return reinterpret_cast<T *>(this->Payload.data()); }
};
} // namespace mi::transam
