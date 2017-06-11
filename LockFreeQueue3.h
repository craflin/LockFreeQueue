
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue3
{
public:
  explicit LockFreeQueue3(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(usize i = 0; i < _capacity; ++i)
    {
      queue[i].tail = i;
      queue[i].head = -1;
    }

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueue3()
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
      if(node->tail != tail)
        return false;
      if(Atomic::compareAndSwap(_tail, tail, tail + 1) == tail)
      {
        new (&node->data)T(data);
        Atomic::swap(node->head, tail);
        return true;
      }
    }
  }

  bool pop(T& result)
  {
    for(;;)
    {
      usize head = _head;
      Node* node = &queue[head % _capacity];
      if(node->head != head)
        return false;
      if(Atomic::compareAndSwap(_head, head, head + 1) == head)
      {
        result = node->data;
        (&node->data)->~T();
        Atomic::swap(node->tail, head + _capacity);
        return true;
      }
    }
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
  Node* queue;
  volatile usize _tail;
  volatile usize _head;
};
