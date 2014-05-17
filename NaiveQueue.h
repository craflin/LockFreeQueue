
#pragma once

#include <nstd/Mutex.h>
#include <nstd/Memory.h>
#include <nstd/Signal.h>

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
  begin:
    mutex.lock();
    if(_occupiedNodes == _capacity)
    {
      writableSignal.reset();
      mutex.unlock();
      //return false; // queue is full
      writableSignal.wait();
      goto begin;
    }
    ++_writeIndex;
    Node& node = queue[_writeIndex % _capacity];
    ASSERT(node.state == Node::free);
    new (&node.data)T(data);
    node.state =  Node::occupied;
    if(++_occupiedNodes == 1)
      readableSignal.set();
    mutex.unlock();
    return true;
  }
  
  bool_t pop(T& result)
  {
  begin:
    mutex.lock();
    if(_occupiedNodes == 0)
    {
      readableSignal.reset();
      mutex.unlock();
      //return false; // queue is empty
      readableSignal.wait();
      goto begin;
    }
    if(_occupiedNodes == _capacity)
      writableSignal.set();
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

  Signal writableSignal;
  Signal readableSignal;
};
