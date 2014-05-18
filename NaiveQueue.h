
#pragma once

#include <nstd/Mutex.h>
#include <nstd/Memory.h>

template <typename T> class NaiveQueue
{
public:
  explicit NaiveQueue(size_t capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);

    _head = 0;
    _tail = 0;
  }

  ~NaiveQueue()
  {
    for(size_t i = _head; i != _tail; ++i)
      (&queue[i % _capacity].data)->~T();
    Memory::free(queue);
  }
  
  size_t capacity() const {return _capacity;}
  
  size_t size() const {return _tail - _head;}
  
  bool_t push(const T& data)
  {
    mutex.lock();
    if(_tail - _head == _capacity)
    {
      mutex.unlock();
      return false; // queue is full
    }
    Node& node = queue[(_tail++) % _capacity];
    new (&node.data)T(data);
    mutex.unlock();
    return true;
  }
  
  bool_t pop(T& result)
  {
    mutex.lock();
    if(_head == _tail)
    {
      mutex.unlock();
      return false; // queue is empty
    }
    Node& node = queue[(_head++) % _capacity];
    result = node.data;
    (&node.data)->~T();
    mutex.unlock();
    return true;
  }

private:
  struct Node
  {
    T data;
  };

private:
  size_t _capacity;
  Node* queue;
  size_t _head;
  size_t _tail;
  Mutex mutex;
};
