
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue
{
public:
  explicit LockFreeQueue(usize capacity)
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
    for(usize i = 0; i < _capacity; ++i)
    {
      _queue[i].tail = i;
      _queue[i].head = -1;
    }

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueue()
  {
    for(usize i = _head; i != _tail; ++i)
      (&_queue[i & _capacityMask].data)->~T();

    Memory::free(_queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize head = _head;
    Atomic::memoryBarrier();
    return _tail - head;
  }
  
  bool push(const T& data)
  {
    Node* node;
    usize next, tail = _tail;
    for(;; tail = next)
    {
      node = &_queue[tail & _capacityMask];
      if(node->tail != tail)
        return false;
      if((next = Atomic::compareAndSwap(_tail, tail, tail + 1)) == tail)
        break;
    }
    new (&node->data)T(data);
    Atomic::swap(node->head, tail);
    return true;
  }

  bool pop(T& result)
  {
    Node* node;
    usize next, head = _head;
    for(;; head = next)
    {
      node = &_queue[head & _capacityMask];
      if(node->head != head)
        return false;
      if((next = Atomic::compareAndSwap(_head, head, head + 1)) == head)
        break;
    }
    result = node->data;
    (&node->data)->~T();
    Atomic::swap(node->tail, head + _capacity);
    return true;
  }

private:
  struct Node
  {
    T data;
    volatile usize tail;
    volatile usize head;
  };

private:
  usize _capacity;
  usize _capacityMask;
  Node* _queue;
  char cacheLinePad1[64];
  volatile usize _tail;
  char cacheLinePad2[64];
  volatile usize _head;
  char cacheLinePad3[64];
};
