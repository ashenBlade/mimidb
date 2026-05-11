#pragma once

#include "trans/CommitSeqNumber.hpp"

namespace mi::storage::trans {

class Snapshot {
  private:
    CommitSeqNumber _csn;

  public:
    Snapshot(CommitSeqNumber csn) : _csn(csn) {};
    CommitSeqNumber CSN() const { return this->_csn; }

    bool IsVisibleFor(CommitSeqNumber csn) const {
        // TODO: тут надо проверять скорее Aborted и т.д. состояния
        return csn < this->_csn;
    }
};

}; // namespace mi::transam
