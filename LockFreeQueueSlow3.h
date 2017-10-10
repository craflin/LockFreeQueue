
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueueSlow3
{
public:
  explicit LockFreeQueueSlow3(usize capacity)
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
      _queue[i].state = 0;

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueueSlow3()
  {
    for(usize i = _head; i != _tail; ++i)
      (&_queue[i & _capacityMask].data)->~T();

    Memory::free(_queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize head = Atomic::load(_head);
    return _tail - head;
  }
  
  bool push(const T& data)
  {
    for(;;)
    {
      usize tail = _tail;
      Node* node = &_queue[tail & _capacityMask];
      switch(Atomic::compareAndSwap(node->state, 0, 2))
      {
      case 2:
        continue;
      case 0:
        break;
      default:
        return false;
      }
      if(Atomic::compareAndSwap(_tail, tail, tail + 1) == tail)
      {
        new (&node->data)T(data);
        Atomic::store(node->state, 1);
        return true;
      }
      else
        node->state = 0;
    }
  }

  bool pop(T& result)
  {
    for(;;)
    {
      usize head = _head;
      Node* node = &_queue[head & _capacityMask];
      switch(Atomic::compareAndSwap(node->state, 1, 3))
      {
      case 3:
        continue;
      case 1:
        break;
      default:
        return false;
      }
      if(Atomic::compareAndSwap(_head, head, head + 1) == head)
      {
        result = node->data;
        (&node->data)->~T();
        Atomic::store(node->state, 0);
        return true;
      }
      else
        node->state = 1;
    }
  }

private:
  struct Node
  {
    T data;
    volatile int state;
  };

private:
  usize _capacityMask;
  Node* _queue;
  usize _capacity;
  char cacheLinePad1[64];
  volatile usize _tail;
  char cacheLinePad2[64];
  volatile usize _head;
  char cacheLinePad3[64];
};
