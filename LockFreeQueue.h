
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueue
{
public:
  explicit LockFreeQueue(size_t capacity) : _capacity(capacity)
  {
    queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(Node* node = queue, * end = queue + _capacity; node < end; ++node)
      node->state = Node::free;

    _freeNodes = capacity;
    _occupiedNodes = 0;
    _writeIndex = -1;
    _safeWriteIndex = -1;
    _readIndex = -1;
    _safeReadIndex = -1;
  }

  ~LockFreeQueue()
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
    size_t freeNodes = _freeNodes;
    if(freeNodes == 0)
      return false; // queue is full
    if(Atomic::compareAndSwap(_freeNodes, freeNodes, freeNodes - 1) != freeNodes)
      goto begin;
    size_t writeIndex = Atomic::increment(_writeIndex);
    Node& node = queue[writeIndex % _capacity];
    ASSERT(node.state == Node::free);
    new (&node.data)T(data);
    //Atomic::swap(node.state, Node::set);
    node.state = Node::set;
  commit:
    size_t safeWriteIndex = _safeWriteIndex;
    size_t nextSafeWriteIndex = safeWriteIndex + 1;
  commitNext:
    if(Atomic::compareAndSwap(queue[nextSafeWriteIndex % _capacity].state, Node::set, Node::occupied) == Node::set)
    {
      if(Atomic::compareAndSwap(_safeWriteIndex, safeWriteIndex, nextSafeWriteIndex) == safeWriteIndex)
      {
        Atomic::increment(_occupiedNodes);
        safeWriteIndex = nextSafeWriteIndex;
        ++nextSafeWriteIndex;
        goto commitNext;
      }
      else
        queue[nextSafeWriteIndex % _capacity].state = Node::set;
      goto commit;
    }
    return true;
  }
  
  bool_t pop(T& result)
  {
  begin:
    size_t occupiedNodes = _occupiedNodes;
    if(occupiedNodes == 0)
      return false; // queue is empty
    if(Atomic::compareAndSwap(_occupiedNodes, occupiedNodes, occupiedNodes - 1) != occupiedNodes)
      goto begin;
    size_t readIndex = Atomic::increment(_readIndex);
    Node& node = queue[readIndex % _capacity];
    ASSERT(node.state == Node::occupied);
    result = node.data;
    (&node.data)->~T();
    //Atomic::swap(node.state, Node::unset);
    node.state = Node::unset;
  release:
    size_t safeReadIndex = _safeReadIndex;
    size_t nextSafeReadIndex = safeReadIndex + 1;
  releaseNext:
    if(Atomic::compareAndSwap(queue[nextSafeReadIndex % _capacity].state, Node::unset, Node::free) == Node::unset)
    {
      if(Atomic::compareAndSwap(_safeReadIndex, safeReadIndex, nextSafeReadIndex) == safeReadIndex)
      {
        Atomic::increment(_freeNodes);
        safeReadIndex = nextSafeReadIndex;
        ++nextSafeReadIndex;
        goto releaseNext;
      }
      else
        queue[nextSafeReadIndex % _capacity].state = Node::unset;
      goto release;
    }
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
  volatile size_t _freeNodes;
  volatile size_t _occupiedNodes;
  volatile size_t _writeIndex;
  volatile size_t _safeWriteIndex;
  volatile size_t _readIndex;
  volatile size_t _safeReadIndex;
};
