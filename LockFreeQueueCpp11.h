
#pragma once

#include <atomic>

template <typename T> class LockFreeQueueCpp11
{
public:
  explicit LockFreeQueueCpp11(usize capacity)
  {
    _capacityMask = capacity - 1;
    _capacityMask |= _capacityMask >> 1;
    _capacityMask |= _capacityMask >> 2;
    _capacityMask |= _capacityMask >> 4;
    _capacityMask |= _capacityMask >> 8;
    _capacityMask |= _capacityMask >> 16;
    _capacityMask |= _capacityMask >> 32;
    _capacity = _capacityMask + 1;

    _queue = (Node*)new char[sizeof(Node) * _capacity];
    for(usize i = 0; i < _capacity; ++i)
    {
      _queue[i].tail.store(i, std::memory_order_relaxed);
      _queue[i].head.store(-1, std::memory_order_relaxed);
    }

    _tail.store(0, std::memory_order_relaxed);
    _head.store(0, std::memory_order_relaxed);
  }

  ~LockFreeQueueCpp11()
  {
    for(usize i = _head; i != _tail; ++i)
      (&_queue[i % _capacity].data)->~T();

    delete [] (char*)_queue;
  }
  
  usize capacity() const {return _capacity;}
  
  usize size() const
  {
    usize head = _head.load(std::memory_order_acquire);
    return _tail.load(std::memory_order_relaxed) - head;
  }
  
  bool push(const T& data)
  {
    Node* node;
    size_t tail = _tail.load(std::memory_order_relaxed);
    for(;;)
    {
      node = &_queue[tail & _capacityMask];
      if(node->tail.load(std::memory_order_relaxed) != tail)
        return false;
      if((_tail.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)))
        break;
    }
    new (&node->data)T(data);
    node->head.store(tail, std::memory_order_release);
    return true;
  }

  bool pop(T& result)
  {
    Node* node;
    size_t head = _head.load(std::memory_order_relaxed);
    for(;;)
    {
      node = &_queue[head & _capacityMask];
      if(node->head.load(std::memory_order_relaxed) != head)
        return false;
      if(_head.compare_exchange_weak(head, head + 1, std::memory_order_relaxed))
        break;
    }
    result = node->data;
    (&node->data)->~T();
    node->tail.store(head + _capacity, std::memory_order_release);
    return true;
  }

private:
  struct Node
  {
    T data;
    std::atomic<size_t> tail;
    std::atomic<size_t> head;
  };

private:
  usize _capacity;
  usize _capacityMask;
  Node* _queue;
  char cacheLinePad1[64];
  std::atomic<size_t> _tail;
  char cacheLinePad2[64];
  std::atomic<size_t> _head;
  char cacheLinePad3[64];
};
