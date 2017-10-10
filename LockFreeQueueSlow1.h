
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeQueueSlow1
{
public:
  explicit LockFreeQueueSlow1(usize capacity)
  {
    _capacityMask = capacity - 1;
    for(usize i = 1; i <= sizeof(void*) * 4; i <<= 1)
      _capacityMask |= _capacityMask >> i;
    _capacity = _capacityMask + 1;

    _queue = (Node*)Memory::alloc(sizeof(Node) * _capacity);
    for(Node* node = _queue, * end = _queue + _capacity; node < end; ++node)
      node->state = Node::free;

    _freeNodes = _capacity;
    _occupiedNodes = 0;
    _writeIndex = -1;
    _safeWriteIndex = -1;
    _readIndex = -1;
    _safeReadIndex = -1;
  }

  ~LockFreeQueueSlow1()
  {
    for(Node* node = _queue, * end = _queue + _capacity; node < end; ++node)
      switch(node->state)
      {
      case Node::set:
      case Node::occupied:
        (&node->data)->~T();
        break;
      default:
        break;
      }
    Memory::free(_queue);
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
    Node* node = &_queue[writeIndex & _capacityMask];
    ASSERT(node->state == Node::free);
    new (&node->data)T(data);
    Atomic::store(node->state, Node::set);
  commit:
    usize safeWriteIndex = _safeWriteIndex;
    usize nextSafeWriteIndex = safeWriteIndex + 1;
  commitNext:
    node = &_queue[nextSafeWriteIndex & _capacityMask];
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
    Node* node = &_queue[readIndex & _capacityMask];
    ASSERT(node->state == Node::occupied);
    result = node->data;
    (&node->data)->~T();
    Atomic::store(node->state, Node::unset);
  release:
    usize safeReadIndex = _safeReadIndex;
    usize nextSafeReadIndex = safeReadIndex + 1;
  releaseNext:
    node = &_queue[nextSafeReadIndex & _capacityMask];
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
  usize _capacityMask;
  Node* _queue;
  usize _capacity;
  volatile usize _freeNodes;
  volatile usize _occupiedNodes;
  volatile usize _writeIndex;
  volatile usize _safeWriteIndex;
  volatile usize _readIndex;
  volatile usize _safeReadIndex;
};
