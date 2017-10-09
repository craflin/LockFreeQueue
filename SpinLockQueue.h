
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class SpinLockQueue
{
public:
  explicit SpinLockQueue(usize capacity)
  {
    _capacityMask = capacity - 1;
    _capacityMask |= _capacityMask >> 1;
    _capacityMask |= _capacityMask >> 2;
    _capacityMask |= _capacityMask >> 4;
    _capacityMask |= _capacityMask >> 8;
    _capacityMask |= _capacityMask >> 16;
    _capacityMask |= _capacityMask >> 32;
    _capacity = _capacityMask + 1;

    _queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    _lock = 0;
    _head = 0;
    _tail = 0;
  }

  ~SpinLockQueue()
  {
    for(usize i = _head; i != _tail; ++i)
      (&_queue[i & _capacityMask].data)->~T();
    Memory::free(_queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize result;
    while(Atomic::testAndSet(_lock) != 0);
    result = _tail - _head;
    Atomic::swap(_lock, 0);
    return result;
  }
  
  bool push(const T& data)
  {
    while(Atomic::testAndSet(_lock) != 0);
    if(_tail - _head == _capacity)
    {
      _lock = 0;
      return false; // queue is full
    }
    Node& node = _queue[(_tail++) & _capacityMask];
    new (&node.data)T(data);
    Atomic::swap(_lock, 0);
    return true;
  }
  
  bool pop(T& result)
  {
    while(Atomic::testAndSet(_lock) != 0);
    if(_head == _tail)
    {
      _lock = 0;
      return false; // queue is empty
    }
    Node& node = _queue[(_head++) & _capacityMask];
    result = node.data;
    (&node.data)->~T();
    Atomic::swap(_lock, 0);
    return true;
  }

private:
  struct Node
  {
    T data;
  };

private:
  usize _capacityMask;
  Node* _queue;
  usize _capacity;
  usize _head;
  usize _tail;
  mutable volatile int32 _lock;
};
