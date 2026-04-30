#include "mimidb.hpp"

#include "cluster_state.hpp"
#include "storage/BufferPin.hpp"
#include "storage/PageTag.hpp"

#include <stdexcept>

using namespace mi::storage;

BufferPin::BufferPin() : _tag(), _buffer(nullptr) {};

BufferPin::BufferPin(PageTag pagetag, std::shared_ptr<Buffer> buffer)
    : _tag(pagetag), _buffer(buffer) { };

// BufferPin BufferPin::GetBuffer(PageTag tag) {
//     auto buffer = BufferPoolGlobal->GetBuffer(tag);
//     return BufferPin{tag, buffer};
// }

std::shared_ptr<Buffer> BufferPin::GetBuffer() {
    return this->_buffer;
}
std::shared_ptr<Buffer> BufferPin::GetBuffer() const {
return this->_buffer;
}

std::byte *BufferPin::GetContents() { return _buffer->GetContents(); }

const std::byte *BufferPin::GetContents() const { return _buffer->GetContents(); }

BufferPin &BufferPin::operator=(BufferPin &&other) {
    if (&other == this) {
        return *this;
    }

    if (this->_buffer != nullptr) {
        BufferPoolGlobal->ReturnBuffer(*this);
    }

    this->_tag = PageTag{};
    this->_buffer = nullptr;

    std::swap(this->_tag, other._tag);
    std::swap(this->_buffer, other._buffer);

    return *this;
}

BufferPin::~BufferPin() {
    if (this->_buffer != nullptr) {
        BufferPoolGlobal->ReturnBuffer(*this);
    }

    this->_buffer = nullptr;
    this->_tag = PageTag{};
}
