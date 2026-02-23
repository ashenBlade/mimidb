#include "transam/Snapshot.hpp"

using namespace mi::transam;

Snapshot::Snapshot(CommitSeqNumber csn) : _csn(csn) {}

CommitSeqNumber Snapshot::CSN() const { return _csn; }
