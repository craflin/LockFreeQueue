
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueueSlow2
{
public:
  explicit LockFreeQueueSlow2(usize capacity)
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
      _queue[i].head = i - 1;
    }

    _tail = 0;
    _head = 0;
  }

  ~LockFreeQueueSlow2()
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
  begin:
    usize tail = _tail;
    Node* node = &_queue[tail & _capacityMask];
    usize newTail = tail + 1;
    usize nodeTail = node->tail;
    if(nodeTail == tail)
    {
      nodeTail = Atomic::compareAndSwap(node->tail, tail, newTail);
      if(nodeTail == tail)
      {
        Atomic::compareAndSwap(_tail, tail, newTail);
        new (&node->data)T(data);
        Atomic::store(node->head, tail);
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
    Node* node = &_queue[head & _capacityMask];
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
        Atomic::store(node->tail, head + _capacity);
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
  usize _capacityMask;
  Node* _queue;
  usize _capacity;
  char cacheLinePad1[64];
  volatile usize _head;
  char cacheLinePad2[64];
  volatile usize _tail;
  char cacheLinePad3[64];
};
