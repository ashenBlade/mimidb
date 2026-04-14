#include "storage/BufferWrapper.hpp"

#include <stdexcept>

using namespace mi::storage;

BufferPin::BufferPin(PageNumber pageno, std::shared_ptr<Buffer> buffer, BufferManager &manager)
    : _pageno(pageno), _buffer(buffer), _manager(manager), _isLocked(false), _lockIsShared(false) {};

BufferPin BufferPin::GetBuffer(PageNumber pageno) {
    auto buffer = manager.GetBuffer(pageno);
    return BufferWrapper{pageno, buffer, manager};
}

std::byte *BufferPin::GetContents() { return _buffer->GetContents(); }

const std::byte *BufferPin::GetContents() const { return _buffer->GetContents(); }

void BufferPin::Lock(bool shared) {
    if (_isLocked)
        throw std::runtime_error("recursive lock is not supported");

    _buffer->Lock(shared);
    _isLocked = true;
    _lockIsShared = shared;
}

void BufferPin::Unlock() {
    if (!_isLocked)
        throw std::runtime_error("buffer is not locked");

    _buffer->Unlock(_lockIsShared);
    _isLocked = _lockIsShared = false;
}

BufferPin &mi::storage::BufferPin::operator=(const BufferPin &other) {
    if (&other == this) {
        return *this;
    }
    // TODO: реализовать copy assignment operator= и попробовать тут swap реализовать (пока не пон че да как)
    std::swap(_pageno, other._pageno);
    std::swap()
}

BufferPin::~BufferPin() {
    if (_pageno == PageNumber::Invalid)
        return;

    if (_isLocked)
        _buffer->Unlock(_lockIsShared);
    _manager.ReturnBuffer(_buffer, _pageno);
    _pageno = PageNumber::Invalid;
}
