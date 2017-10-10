
#pragma once

#include <nstd/Mutex.h>
#include <nstd/Memory.h>

template <typename T> class MutexLockQueue
{
public:
  explicit MutexLockQueue(usize capacity)
  {
    _capacityMask = capacity - 1;
    for(usize i = 1; i <= sizeof(void*) * 4; i <<= 1)
      _capacityMask |= _capacityMask >> i;
    _capacity = _capacityMask + 1;

    _queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    _head = 0;
    _tail = 0;
  }

  ~MutexLockQueue()
  {
    for(usize i = _head; i != _tail; ++i)
      (&_queue[i & _capacityMask].data)->~T();
    Memory::free(_queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize result;
    _mutex.lock();
    result = _tail - _head;
    _mutex.unlock();
    return result;
  }
  
  bool push(const T& data)
  {
    _mutex.lock();
    if(_tail - _head == _capacity)
    {
      _mutex.unlock();
      return false; // queue is full
    }
    Node& node = _queue[(_tail++) & _capacityMask];
    new (&node.data)T(data);
    _mutex.unlock();
    return true;
  }
  
  bool pop(T& result)
  {
    _mutex.lock();
    if(_head == _tail)
    {
      _mutex.unlock();
      return false; // queue is empty
    }
    Node& node = _queue[(_head++) & _capacityMask];
    result = node.data;
    (&node.data)->~T();
    _mutex.unlock();
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
  mutable Mutex _mutex;
};
