
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class SpinLockQueue
{
public:
  explicit SpinLockQueue(size_t capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    lock = 0;
    _head = 0;
    _tail = 0;
  }

  ~SpinLockQueue()
  {
    for(size_t i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();
    Memory::free(queue);
  }
  
  size_t capacity() const {return _capacity;}
  
  size_t size() const {return _tail - _head;}
  
  bool_t push(const T& data)
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
  
  bool_t pop(T& result)
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
  size_t _capacity;
  Node* queue;
  size_t _head;
  size_t _tail;
  volatile int32_t lock;
};
