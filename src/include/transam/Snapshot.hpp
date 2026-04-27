#pragma once

#include "transam/CommitSeqNumber.hpp"

namespace mi::transam {

class Snapshot {
  private:
    CommitSeqNumber _csn;

  public:
    Snapshot(CommitSeqNumber csn) : _csn(csn) {};
    CommitSeqNumber CSN() const { return this->_csn; }
    
    bool IsVisibleFor(CommitSeqNumber csn) const {
      return csn < this->_csn;
    }
};

}; // namespace mi::transam
