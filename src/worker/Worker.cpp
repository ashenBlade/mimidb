#include "mimidb.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

#include "cluster_state.hpp"
#include "worker/Worker.hpp"
#include "worker/Handler.hpp"

using namespace mi::worker;

Worker::Worker() : Worker(WorkerId::Invalid) {};

Worker::Worker(WorkerId id) : _id(id) { assert(id.IsValid()); };

Worker::Worker(Worker &&other) noexcept { this->swap(other); }

void Worker::swap(Worker &other) noexcept {
    std::swap(this->_id, other._id);
}

Worker &Worker::operator=(Worker &&other) noexcept {
    this->swap(other);
    return *this;
}

WorkerId Worker::GetId() const { return this->_id; }

bool Worker::IsBusy() const {
    return this->_busy;
}

void Worker::HandleUserConnectionGuts(WorkerId id, int sock) {
    try {
        mi::worker::HandleUserConnection(id, sock);
    } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
    }

    // Mark yourself free
    WorkerGlobal->GetWorker(id)->_busy = false;
}

void Worker::Submit(int sock) {
    if (this->_busy) {
        throw std::logic_error("Can not submit handler because worker already running");
    }
    
    // There already was some execution earlier
    if (this->_thread.joinable()) {
        this->_thread.join();
    }

    this->_busy = true;
    this->_thread = std::thread{Worker::HandleUserConnectionGuts, this->_id, sock};
}
