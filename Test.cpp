
#include <nstd/Console.h>
#include <nstd/Debug.h>
#include <nstd/Thread.h>
#include <nstd/List.h>
#include <nstd/Time.h>

#include "LockFreeQueue2.h"

//#include "NaiveQueue.h"
//#define LockFreeQueue NaiveQueue

uint_t producerThread(void_t* param)
{
  LockFreeQueue<int_t>* queue = (LockFreeQueue<int_t>*)param;
  for(int i = 0; i < 100000; ++i)
  {
    while(!queue->push(12))
      Thread::yield();
  }
  return 0;
}

uint_t consumerThread(void_t* param)
{
  LockFreeQueue<int_t>* queue = (LockFreeQueue<int_t>*)param;
  int_t val;
  for(int i = 0; i < 100000; ++i)
  {
    while(!queue->pop(val))
      Thread::yield();
  }
  return 0;
}

int_t main(int_t argc, char_t* argv[])
{
  Console::printf(_T("%s\n"), _T("Testing..."));

  volatile int32_t int32 = 0;
  volatile uint32_t uint32 = 0;
  ASSERT(Atomic::compareAndSwap(int32, 0, 1) == 0);
  ASSERT(int32 == 1);
  ASSERT(Atomic::compareAndSwap(uint32, 0, 1) == 0);
  ASSERT(uint32 == 1);

  volatile int64_t int64 = 0;
  volatile uint64_t uint64 = 0;
  ASSERT(Atomic::compareAndSwap(int64, 0, 1) == 0);
  ASSERT(int64 == 1);
  ASSERT(Atomic::compareAndSwap(uint64, 0, 1) == 0);
  ASSERT(uint64 == 1);

  {
    LockFreeQueue<int_t> queue(10000);
    int_t result;
    ASSERT(!queue.pop(result));
    ASSERT(queue.push(42));
    ASSERT(queue.pop(result));
    ASSERT(result == 42);
    ASSERT(!queue.pop(result));
  }

  //{
  //  LockFreeQueue<int_t> queue(0);
  //  int_t result;
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //}

  {
    LockFreeQueue<int_t> queue(2);
    int_t result;
    ASSERT(!queue.pop(result));
    ASSERT(queue.push(42));
    ASSERT(queue.push(43));
    ASSERT(!queue.push(44));
    ASSERT(queue.pop(result));
    ASSERT(result == 42);
    ASSERT(queue.pop(result));
    ASSERT(result == 43);
    ASSERT(!queue.pop(result));
    ASSERT(queue.push(44));
    ASSERT(queue.push(45));
    ASSERT(!queue.push(46));
    ASSERT(queue.pop(result));
    ASSERT(result == 44);
    ASSERT(queue.push(47));
    ASSERT(!queue.push(48));
  }


  timestamp_t startTime = Time::ticks();
  {
    LockFreeQueue<int_t> queue(10000);
    List<Thread*> threads;
    for(int_t i = 0; i < 60; ++i)
    {
      Thread* thread = new Thread;
      thread->start(producerThread, &queue);
      threads.append(thread);
    }
    for(int_t i = 0; i < 60; ++i)
    {
      Thread* thread = new Thread;
      thread->start(consumerThread, &queue);
      threads.append(thread);
    }
    for(List<Thread*>::Iterator i = threads.begin(), end = threads.end(); i != end; ++i)
    {
      Thread* thread = *i;
      thread->join();
      delete thread;
    }
  }
  timestamp_t duration = Time::ticks() - startTime;
  Console::printf(_T("it took %lld ms\n"), duration);
}
