
#pragma once

#include <nstd/Mutex.h>
#include <nstd/Memory.h>

template <typename T> class NaiveQueue
{
public:
  explicit NaiveQueue(size_t capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(Node* node = queue, * end = queue + _capacity; node < end; ++node)
      node->state = Node::free;

    _occupiedNodes = 0;
    _writeIndex = -1;
    _readIndex = -1;
  }

  ~NaiveQueue()
  {
    for(Node* node = queue, * end = queue + _capacity; node < end; ++node)
      switch(node->state)
      {
      case Node::set:
      case Node::occupied:
        (&node->data)->~T();
        break;
      default:
        break;
      }
    Memory::free(queue);
  }
  
  size_t capacity() const {return _capacity;}
  
  size_t size() const {return _capacity - _freeNodes;}
  
  bool_t push(const T& data)
  {
    mutex.lock();
    if(_occupiedNodes == _capacity)
    {
      mutex.unlock();
      return false; // queue is full
    }
    ++_writeIndex;
    Node& node = queue[_writeIndex % _capacity];
    ASSERT(node.state == Node::free);
    new (&node.data)T(data);
    node.state =  Node::occupied;
    ++_occupiedNodes;
    mutex.unlock();
    return true;
  }
  
  bool_t pop(T& result)
  {
    mutex.lock();
    if(_occupiedNodes == 0)
    {
      mutex.unlock();
      return false; // queue is empty
    }
    --_occupiedNodes;
    ++_readIndex;
    Node& node = queue[_readIndex % _capacity];
    ASSERT(node.state == Node::occupied);
    result = node.data;
    (&node.data)->~T();
    node.state = Node::free;
    mutex.unlock();
    return true;
  }

private:
  struct Node
  {
    T data;
    enum State
    {
      free,
      set,
      occupied,
      unset,
    };
    volatile int_t state;
  };

private:
  size_t _capacity;
  Node* queue;
  size_t _occupiedNodes;
  size_t _writeIndex;
  size_t _readIndex;
  Mutex mutex;
};
