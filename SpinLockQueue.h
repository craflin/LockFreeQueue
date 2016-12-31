
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class SpinLockQueue
{
public:
  explicit SpinLockQueue(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    lock = 0;
    _head = 0;
    _tail = 0;
  }

  ~SpinLockQueue()
  {
    for(usize i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();
    Memory::free(queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const {return _tail - _head;}
  
  bool push(const T& data)
  {
    while(Atomic::testAndSet(lock) != 0);
    if(_tail - _head == _capacity)
    {
      lock = 0;
      return false; // queue is full
    }
    Node& node = queue[(_tail++) % _capacity];
    new (&node.data)T(data);
    // memory barrier?
    lock = 0;
    return true;
  }
  
  bool pop(T& result)
  {
    while(Atomic::testAndSet(lock) != 0);
    if(_head == _tail)
    {
      lock = 0;
      return false; // queue is empty
    }
    Node& node = queue[(_head++) % _capacity];
    result = node.data;
    (&node.data)->~T();
    // memory barrier?
    lock = 0;
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
  volatile int32 lock;
};
