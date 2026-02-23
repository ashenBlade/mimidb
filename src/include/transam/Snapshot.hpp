#pragma once

#include "transam/CommitSeqNumber.hpp"

namespace mi::transam {

class Snapshot {
private:
    CommitSeqNumber _csn;

public:
    Snapshot(CommitSeqNumber csn);
    CommitSeqNumber CSN() const;
};

};
