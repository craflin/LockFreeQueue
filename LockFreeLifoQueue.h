
#pragma once

#include <nstd/Atomic.h>
#include <nstd/Memory.h>

template <typename T> class LockFreeLifoQueue
{
public:
  explicit LockFreeLifoQueue(usize capacity)
    : _capacity(capacity)
  {
    _indexMask = capacity;
    _indexMask |= _indexMask >> 1;
    _indexMask |= _indexMask >> 2;
    _indexMask |= _indexMask >> 4;
    _indexMask |= _indexMask >> 8;
    _indexMask |= _indexMask >> 16;
    _indexMask |= _indexMask >> 32;
    _abaOffset = _indexMask + 1;

    _queue = (Node*)Memory::alloc(sizeof(Node) * (capacity + 1));
    for(usize i = 1; i < capacity;)
    {
      Node& node = _queue[i];
      node.abaNextFree = ++i;
    }
    _queue[capacity].abaNextFree = 0;

    _abaFree = 1;
    _abaPushed = 0;
  }

  ~LockFreeLifoQueue()
  {
    for(usize abaPushed = _abaPushed;;)
    {
      usize nodeIndex = abaPushed & _indexMask;
      if(!nodeIndex)
        break;
      Node& node = _queue[nodeIndex];
      abaPushed = node.abaNextPushed;
      (&node.data)->~T();
    }

    Memory::free(_queue);
  }
  
  usize capacity() const {return _capacity;}

  usize size() const {return 0;}

  bool push(const T& data)
  {
    Node* node;
    usize abaFree;
    for(;;)
    {
      abaFree = _abaFree;
      usize nodeIndex = abaFree & _indexMask;
      if(!nodeIndex)
        return false;
      node = &_queue[nodeIndex];
      if(Atomic::compareAndSwap(_abaFree, abaFree, node->abaNextFree + _abaOffset) == abaFree)
        break;
    }

    new (&node->data)T(data);
    for(;;)
    {
      usize abaPushed = _abaPushed;
      node->abaNextPushed = abaPushed;
      if(Atomic::compareAndSwap(_abaPushed, abaPushed, abaFree) == abaPushed)
        return true;
    }
  }

  bool pop(T& result)
  {
    Node* node;
    usize abaPushed;
    for(;;)
    {
      abaPushed = _abaPushed;
      usize nodeIndex = abaPushed & _indexMask;
      if(!nodeIndex)
        return false;
      node = &_queue[nodeIndex];
      if(Atomic::compareAndSwap(_abaPushed, abaPushed, node->abaNextPushed + _abaOffset) == abaPushed)
        break;
    }

    result = node->data;
    (&node->data)->~T();
    abaPushed += _abaOffset;
    for(;;)
    {
      usize abaFree = _abaFree;
      node->abaNextFree = abaFree;
      if(Atomic::compareAndSwap(_abaFree, abaFree, abaPushed) == abaFree)
        return true;
    }
  }

private:
  struct Node
  {
    T data;
    volatile usize abaNextFree;
    volatile usize abaNextPushed;
  };

private:
  usize _indexMask;
  Node* _queue;
  usize _abaOffset;
  usize _capacity;
  char cacheLinePad1[64];
  volatile usize _abaFree;
  char cacheLinePad2[64];
  volatile usize _abaPushed;
  char cacheLinePad3[64];
};
