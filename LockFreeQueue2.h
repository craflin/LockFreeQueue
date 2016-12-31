
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue2
{
public:
  explicit LockFreeQueue2(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(usize i = 0; i < capacity; ++i)
      queue[i].ready = false;

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueue2()
  {
    for(usize i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();

    Memory::free(queue);
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize head = _head;
    // memory barrier?
    return _tail - head;
  }
  
  bool push(const T& data)
  {
    for(;;)
    {
      usize tail = _tail;
      Node* node = &queue[tail % _capacity];
      if(node->ready)
        return false;
      if(Atomic::compareAndSwap(_tail, tail, tail + 1) == tail)
      {
        new (&node->data)T(data);
        // memory barrier?
        node->ready = true;
        return true;
      }
      else
        return false;
    }
  }

  bool pop(T& result)
  {
    for(;;)
    {
      usize head = _head;
      Node* node = &queue[head % _capacity];
      if(!node->ready)
        return false;
      if(Atomic::compareAndSwap(_head, head, head + 1) == head)
      {
        result = node->data;
        (&node->data)->~T();
        // memory barrier?
        node->ready = false;
        return true;
      }
      else
        return false;
    }
  }

private:
  struct Node
  {
    T data;
    bool ready;
  };

private:
  usize _capacity;
  Node* queue;
  usize _tail;
  usize _head;
};
