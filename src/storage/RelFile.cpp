#include "mimidb.hpp"

#include "storage/RelFile.hpp"

#include <format>
#include <stdexcept>
#include <utility>

using namespace mi::storage;

RelFile::RelFile(File &&file) : _file(std::move(file)) {}

RelFile::RelFile(RelFile &&other) noexcept { this->_file = std::move(other._file); }

RelFile &RelFile::operator=(RelFile &&other) noexcept {
    assert(this != &other);

    this->_file.Close();
    std::swap(this->_file, other._file);

    return *this;
}

RelFile::~RelFile() { this->Close(); }

void RelFile::Write(const std::byte *buffer, PageNumber pageno) {
    if (!pageno.IsValid()) {
        throw std::invalid_argument("could not write page: pageno is invalid");
    }

    auto offset = PAGESIZE * pageno;
    auto size = static_cast<size_t>(PAGESIZE);

    this->_file.Write(buffer, size, offset);
}

void RelFile::Extend(PageNumber pageno) {
    std::array<std::byte, PAGESIZE> buffer {};
    buffer.fill(std::byte{0});
    this->Write(buffer.data(), pageno);
}

void RelFile::Read(std::byte *buffer, PageNumber pageno) {
    if (!pageno.IsValid()) {
        throw std::invalid_argument("could not read page: pageno is invalid");
    }

    auto offset = PAGESIZE * static_cast<off64_t>(pageno);
    auto size = static_cast<size_t>(PAGESIZE);

    auto ret = this->_file.Read(buffer, size, offset);
    if (ret != PAGESIZE) {
        throw std::runtime_error("could not read page: read only " + std::to_string(ret) +
                                 " bytes");
    }
}

void RelFile::Flush() { this->_file.Fsync(); }

PageNumber RelFile::GetPagesCount() {
    auto size = this->_file.Size();
    return PageNumber{static_cast<PageNumber::type>(size / PAGESIZE)};
}

void RelFile::Close() { this->_file.Close(); }

RelFile RelFile::Open(Oid relid, int mode) {
    auto filepath = std::format("data/{}", relid.value);
    return RelFile{File::Open(filepath, mode)};
}

