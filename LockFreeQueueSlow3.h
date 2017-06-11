
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueueSlow3
{
public:
  explicit LockFreeQueueSlow3(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(usize i = 0; i < capacity; ++i)
      queue[i].state = 0;

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueueSlow3()
  {
    for(usize i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();

    Memory::free(queue);
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
    for(;;)
    {
      usize tail = _tail;
      Node* node = &queue[tail % _capacity];
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
        Atomic::swap(node->state, 1);
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
      Node* node = &queue[head % _capacity];
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
        Atomic::swap(node->state, 0);
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
  usize _capacity;
  Node* queue;
  volatile usize _tail;
  volatile usize _head;
};
