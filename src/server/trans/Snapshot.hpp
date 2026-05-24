#pragma once

#include "storage/undo/UndoSeqNumber.hpp"
#include "trans/CommitSeqNumber.hpp"

namespace mi::storage::trans {

class Snapshot {
  private:
    // Snapshot commit seq number below which all CSN are visible
    CommitSeqNumber _csn;
    // Command Id separating different statements in same transaction.
    // All tuples created by same transaction who have usn greater or equal to _cid
    // are invisible, because created by same statement (i.e. insert scan).
    undo::UndoSeqNumber _cid;

  public:
    Snapshot(CommitSeqNumber csn, undo::UndoSeqNumber cid) : _csn(csn), _cid(cid) {}
    CommitSeqNumber CSN() const { return this->_csn; }
    undo::UndoSeqNumber CommandId() const { return this->_cid; }
};

}; // namespace mi::storage::trans
