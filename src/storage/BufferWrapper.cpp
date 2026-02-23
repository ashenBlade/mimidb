#include "storage/BufferWrapper.hpp"

#include <stdexcept>

using namespace mi::storage;

BufferWrapper::BufferWrapper(PageNumber pageno, std::shared_ptr<Buffer> buffer, BufferManager &manager)
    : _pageno(pageno), _buffer(buffer), _manager(manager), _isLocked(false), _lockIsShared(false) {};

std::byte *BufferWrapper::GetContents() { return _buffer->GetContents(); }

const std::byte *BufferWrapper::GetContents() const { return _buffer->GetContents(); }

void BufferWrapper::Lock(bool shared) {
    if (_isLocked)
        throw std::runtime_error("recursive lock is not supported");

    _buffer->Lock(shared);
    _isLocked = true;
    _lockIsShared = shared;
}

void BufferWrapper::Unlock() {
    if (!_isLocked)
        throw std::runtime_error("buffer is not locked");

    _buffer->Unlock(_lockIsShared);
    _isLocked = _lockIsShared = false;
}

BufferWrapper::~BufferWrapper() {
    _manager.ReturnBuffer(_buffer, _pageno);
}

