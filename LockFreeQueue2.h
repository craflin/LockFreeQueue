
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue
{
public:
  explicit LockFreeQueue(size_t capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(size_t i = 0; i < capacity; ++i)
    {
      queue[i].tail = i;
      queue[i].head = i - 1;
    }

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueue()
  {
    for(size_t i = 0, head = _head; i < _capacity; ++i, ++head)
    {
      Node* node = &queue[head % _capacity];
      if(node->head == head)
        (&node->data)->~T();
    }

    Memory::free(queue);
  }
  
  size_t capacity() const {return _capacity;}
  
  size_t size() const {return _capacity - _freeNodes;}
  
  bool_t push(const T& data)
  {
  begin:
    size_t tail = _tail;
    Node* node = &queue[tail % _capacity];
    size_t newTail = tail + 1;
    size_t nodeTail = node->tail;
    if(nodeTail == tail)
    {
      nodeTail = Atomic::compareAndSwap(node->tail, tail, newTail);
      if(nodeTail == tail)
      {
        Atomic::compareAndSwap(_tail, tail, newTail);
        new (&node->data)T(data);
        // memory barrier?
        node->head = tail;
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

  bool_t pop(T& result)
  {
  begin:
    size_t head = _head;
    Node* node = &queue[head % _capacity];
    size_t newHead = head + 1;
    size_t nodeHead = node->head;
    if(nodeHead == head)
    {
      nodeHead = Atomic::compareAndSwap(node->head, head, newHead);
      if(nodeHead == head)
      {
        Atomic::compareAndSwap(_head, head, newHead);
        result = node->data;
        (&node->data)->~T();
        // memory barrier?
        node->tail = head + _capacity;
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
    volatile size_t head;
    volatile size_t tail;
  };

private:
  size_t _capacity;
  Node* queue;
  volatile size_t _head;
  volatile size_t _tail;
};
