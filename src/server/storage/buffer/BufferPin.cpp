#include "storage/buffer/BufferPin.hpp"
#include "cluster_state.hpp"
#include "storage/buffer/Buffer.hpp"
#include "storage/buffer/PageTag.hpp"

using namespace mi::storage::buffer;

BufferPin::BufferPin() : _tag(), _buffer(Buffer::Invalid()) {};

BufferPin::BufferPin(PageTag pagetag, Buffer buffer)
    : _tag(pagetag), _buffer(buffer) {};

BufferPin::BufferPin(BufferPin &&other) {
    assert(&other != this);

    if (this->IsValid()) {
        BufferPoolGlobal->ReturnBuffer(this->_buffer);
    }

    std::swap(this->_tag, other._tag);
    std::swap(this->_buffer, other._buffer);
}

BufferPin &BufferPin::operator=(BufferPin &&other) {
    assert(&other != this);

    if (this->IsValid()) {
        BufferPoolGlobal->ReturnBuffer(this->_buffer);
    }

    std::swap(this->_tag, other._tag);
    std::swap(this->_buffer, other._buffer);

    return *this;
}

BufferPin::~BufferPin() {
    if (this->IsValid()) {
        BufferPoolGlobal->ReturnBuffer(this->_buffer);
    }

    this->_buffer = Buffer::Invalid();
}
