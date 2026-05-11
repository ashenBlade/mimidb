#pragma once

#include "access/table/Oid.hpp"
#include "storage/buffer/PageNumber.hpp"

namespace mi::storage::buffer {
struct PageTag {
  public:
    // Relation id
    Oid Relid;
    // Page number
    PageNumber PageNo;

    PageTag() : Relid(Oid::Invalid), PageNo(PageNumber::Invalid) {};
    PageTag(Oid relid, PageNumber pageno) : Relid(relid), PageNo(pageno) {};

    PageTag(const PageTag &other) = default;
    PageTag &operator=(const PageTag &other) = default;

    PageTag(PageTag &&other) = default;
    PageTag &operator=(PageTag &&other) = default;

    bool operator==(const PageTag &other) const {
        return this->Relid == other.Relid && this->PageNo == other.PageNo;
    }
};

// Used only std::hash support
struct PageTagHash {
    size_t operator()(const PageTag &tag) const noexcept {
        return std::hash<Oid>()(tag.Relid) ^ (std::hash<PageNumber>()(tag.PageNo) << 1);
    }
};
} // namespace mi::storage