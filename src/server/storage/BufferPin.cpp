#include "mimidb.hpp"

#include "cluster_state.hpp"
#include "storage/buffer/BufferPin.hpp"
#include "storage/buffer/PageTag.hpp"

using namespace mi::storage;

BufferPin::BufferPin() : _tag(), _buffer(nullptr) {};

BufferPin::BufferPin(PageTag pagetag, std::shared_ptr<Buffer> buffer)
    : _tag(pagetag), _buffer(buffer) { };

BufferPin::BufferPin(BufferPin &&other) {
    assert(&other != this);

    if (this->_buffer != nullptr) {
        BufferPoolGlobal->ReturnBuffer(*this);
    }

    this->_tag = PageTag{};
    this->_buffer = nullptr;

    std::swap(this->_tag, other._tag);
    std::swap(this->_buffer, other._buffer);
}

BufferPin &BufferPin::operator=(BufferPin &&other) {
    assert(&other != this);

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
    if (this->IsValid()) {
        BufferPoolGlobal->ReturnBuffer(*this);
    }

    this->_buffer = nullptr;
}
