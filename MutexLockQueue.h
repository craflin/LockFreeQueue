
#pragma once

#include <nstd/Mutex.h>
#include <nstd/Memory.h>

template <typename T> class MutexLockQueue
{
public:
  explicit MutexLockQueue(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    _head = 0;
    _tail = 0;
  }

  ~MutexLockQueue()
  {
    for(usize i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();
    Memory::free(queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const {return _tail - _head;}
  
  bool push(const T& data)
  {
    mutex.lock();
    if(_tail - _head == _capacity)
    {
      mutex.unlock();
      return false; // queue is full
    }
    Node& node = queue[(_tail++) % _capacity];
    new (&node.data)T(data);
    mutex.unlock();
    return true;
  }
  
  bool pop(T& result)
  {
    mutex.lock();
    if(_head == _tail)
    {
      mutex.unlock();
      return false; // queue is empty
    }
    Node& node = queue[(_head++) % _capacity];
    result = node.data;
    (&node.data)->~T();
    mutex.unlock();
    return true;
  }

private:
  struct Node
  {
    T data;
  };

private:
  usize _capacity;
  Node* queue;
  usize _head;
  usize _tail;
  Mutex mutex;
};
