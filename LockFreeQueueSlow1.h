
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueueSlow1
{
public:
  explicit LockFreeQueueSlow1(usize capacity) : _capacity(capacity)
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

  ~LockFreeQueueSlow1()
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
  
  usize capacity() const {return _capacity;}
  
  usize size() const {return _capacity - _freeNodes;}
  
  bool push(const T& data)
  {
  begin:
    usize freeNodes = _freeNodes;
    if(freeNodes == 0)
      return false; // queue is full
    if(Atomic::compareAndSwap(_freeNodes, freeNodes, freeNodes - 1) != freeNodes)
      goto begin;
    usize writeIndex = Atomic::increment(_writeIndex);
    Node* node = &queue[writeIndex % _capacity];
    ASSERT(node->state == Node::free);
    new (&node->data)T(data);
    Atomic::swap(node->state, Node::set);
  commit:
    usize safeWriteIndex = _safeWriteIndex;
    usize nextSafeWriteIndex = safeWriteIndex + 1;
  commitNext:
    node = &queue[nextSafeWriteIndex % _capacity];
    if(node->state == Node::set && Atomic::compareAndSwap(node->state, Node::set, Node::occupied) == Node::set)
    {
      if(Atomic::compareAndSwap(_safeWriteIndex, safeWriteIndex, nextSafeWriteIndex) == safeWriteIndex)
      {
        Atomic::increment(_occupiedNodes);
        safeWriteIndex = nextSafeWriteIndex;
        ++nextSafeWriteIndex;
        goto commitNext;
      }
      else
        node->state = Node::set;
      goto commit;
    }
    return true;
  }
  
  bool pop(T& result)
  {
  begin:
    usize occupiedNodes = _occupiedNodes;
    if(occupiedNodes == 0)
      return false; // queue is empty
    if(Atomic::compareAndSwap(_occupiedNodes, occupiedNodes, occupiedNodes - 1) != occupiedNodes)
      goto begin;
    usize readIndex = Atomic::increment(_readIndex);
    Node* node = &queue[readIndex % _capacity];
    ASSERT(node->state == Node::occupied);
    result = node->data;
    (&node->data)->~T();
    Atomic::swap(node->state, Node::unset);
  release:
    usize safeReadIndex = _safeReadIndex;
    usize nextSafeReadIndex = safeReadIndex + 1;
  releaseNext:
    node = &queue[nextSafeReadIndex % _capacity];
    if(node->state == Node::unset && Atomic::compareAndSwap(node->state, Node::unset, Node::free) == Node::unset)
    {
      if(Atomic::compareAndSwap(_safeReadIndex, safeReadIndex, nextSafeReadIndex) == safeReadIndex)
      {
        Atomic::increment(_freeNodes);
        safeReadIndex = nextSafeReadIndex;
        ++nextSafeReadIndex;
        goto releaseNext;
      }
      else
        node->state = Node::unset;
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
    volatile int state;
  };

private:
  usize _capacity;
  Node* queue;
  volatile usize _freeNodes;
  volatile usize _occupiedNodes;
  volatile usize _writeIndex;
  volatile usize _safeWriteIndex;
  volatile usize _readIndex;
  volatile usize _safeReadIndex;
};
