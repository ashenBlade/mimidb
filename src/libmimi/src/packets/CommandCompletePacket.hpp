#pragma once

#include "packets/IPacket.hpp"
#include <string>
namespace mi::interface::libmimi {
class CommandCompletePacket : public IPacket {
  private:
    // Command tag on result
    std::string _tag;

    // Number of rows affected by query.
    //
    // Interpretation depends on tag: for SELECT it will be total rows count
    // and for INSERT/UPDATE/DELETE total affected rows.
    //
    // If -1 then value is absent (not set)
    int64_t _rows;

  public:
    CommandCompletePacket(std::string tag, int64_t rows)
        : IPacket(PacketType::CommandComplete), _tag(std::move(tag)), _rows(rows) {};

    const std::string &GetTag() const;
    int64_t GetRowsCount() const;
    void Accept(IPacketVisitor &visitor) const;
};
} // namespace mi::interface::libmimi
