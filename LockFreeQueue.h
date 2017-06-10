
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue
{
public:
  explicit LockFreeQueue(usize capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(usize i = 0; i < capacity; ++i)
    {
      queue[i].tail = i;
      queue[i].head = i - 1;
    }

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueue()
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
  begin:
    usize tail = _tail;
    Node* node = &queue[tail % _capacity];
    usize newTail = tail + 1;
    usize nodeTail = node->tail;
    if(nodeTail == tail)
    {
      nodeTail = Atomic::compareAndSwap(node->tail, tail, newTail);
      if(nodeTail == tail)
      {
        Atomic::compareAndSwap(_tail, tail, newTail);
        new (&node->data)T(data);
        Atomic::swap(node->head, tail);
        return true;
      }
    }
    if(nodeTail == newTail)
    {
      Atomic::compareAndSwap(_tail, tail, newTail);
      goto begin;
    }
    else
      return false;
  }

  bool pop(T& result)
  {
  begin:
    usize head = _head;
    Node* node = &queue[head % _capacity];
    usize newHead = head + 1;
    usize nodeHead = node->head;
    if(nodeHead == head)
    {
      nodeHead = Atomic::compareAndSwap(node->head, head, newHead);
      if(nodeHead == head)
      {
        Atomic::compareAndSwap(_head, head, newHead);
        result = node->data;
        (&node->data)->~T();
        Atomic::swap(node->tail, head + _capacity);
        return true;
      }
    }
    if(nodeHead == newHead)
    {
      Atomic::compareAndSwap(_head, head, newHead);
      goto begin;
    }
    else
      return false;
  }

private:
  struct Node
  {
    T data;
    volatile usize head;
    volatile usize tail;
  };

private:
  usize _capacity;
  Node* queue;
  volatile usize _head;
  volatile usize _tail;
};
