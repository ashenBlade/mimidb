#pragma once

#include "worker/WorkerId.hpp"

namespace mi::worker {
class WorkerListNode {
  public:
    WorkerId next;
    WorkerId prev;
};

// forward declaration for Iterator
class Worker;

class WorkerListIterator {
  private:
    /// @brief WorkerId we are observing now
    WorkerId _current;
    /// @brief Next WorkerId to seek
    WorkerId _next;

  public:
    WorkerListIterator(WorkerId start);

    Worker &operator *();

    /// @brief Iterate to next entry in list
    /// @return true if success, otherwise false - list ended
    bool Next();
};

class WorkerListHead {
  public:
    WorkerId head;
    WorkerId tail;

    /// @brief Push new entry to tail of list
    /// @param id WorkerId to add
    void Push(WorkerId id);
    /// @brief Delete worker from list
    /// @param id WorkerId to remove
    void Delete(WorkerId id);
    bool IsEmpty() const;

    /// @brief Begin iteration of list
    /// @return Iterator object
    WorkerListIterator Iterate();
};


}; // namespace mi::worker
