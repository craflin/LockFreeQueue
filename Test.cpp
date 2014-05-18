
#include <nstd/Console.h>
#include <nstd/Debug.h>
#include <nstd/Thread.h>
#include <nstd/List.h>
#include <nstd/Time.h>

#include "LockFreeQueue.h"
#include "LockFreeQueueSlow1.h"
#include "MutexLockQueue.h"
#include "SpinLockQueue.h"

static const int testItems = 1000000 * 16;
static const int testThreadConsumerThreads = 4;
static const int testThreadProducerThreads = 4;
static const int testItemsPerConsumerThread = testItems / testThreadConsumerThreads;
static const int testItemsPerProducerThread = testItems / testThreadProducerThreads;

template<typename T> class IQueue
{
public:
  virtual size_t size() const = 0;
  virtual bool_t push(const T& data) = 0;
  virtual bool_t pop(T& result) = 0;
};

template<typename T, class Q> class TestQueue : public IQueue<T>
{
public:
  TestQueue(size_t size) : queue(size) {}
  size_t size() const {return queue.size();}
  bool_t push(const T& data) {return queue.push(data);}
  bool_t pop(T& result) {return queue.pop(result);}
private:
  Q queue;
};

volatile size_t producerSum;
volatile size_t consumerSum;

uint_t producerThread(void_t* param)
{
  IQueue<int_t>* queue = (IQueue<int_t>*)param;
  for(int_t i = 0; i < testItemsPerProducerThread; ++i)
  {
    while(!queue->push(i))
      Thread::yield();
    Atomic::fetchAndAdd(producerSum, i);
  }
  return 0;
}

uint_t consumerThread(void_t* param)
{
  IQueue<int_t>* queue = (IQueue<int_t>*)param;
  int_t val;
  for(int_t i = 0; i < testItemsPerConsumerThread; ++i)
  {
    while(!queue->pop(val))
      Thread::yield();
    Atomic::fetchAndAdd(consumerSum, val);
  }
  return 0;
}

template<class Q> void_t testQueue(const String& name)
{
  Console::printf(_T("Testing %s... "), (const tchar_t*)name);

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
    TestQueue<int_t, Q> queue(10000);
    int_t result;
    ASSERT(!queue.pop(result));
    ASSERT(queue.push(42));
    ASSERT(queue.pop(result));
    ASSERT(result == 42);
    ASSERT(!queue.pop(result));
  }

  //{
  //  TestQueue<int_t, Q> queue(0);
  //  int_t result;
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //}

  {
    TestQueue<int_t, Q> queue(2);
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

  producerSum = 0;
  consumerSum = 0;

  timestamp_t startTime = Time::ticks();
  {
    TestQueue<int_t, Q> queue(10000);
    List<Thread*> threads;
    for(int_t i = 0; i < testThreadProducerThreads; ++i)
    {
      Thread* thread = new Thread;
      thread->start(producerThread, &queue);
      threads.append(thread);
    }
    for(int_t i = 0; i < testThreadConsumerThreads; ++i)
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
    ASSERT(queue.size() == 0);
  }
  timestamp_t duration = Time::ticks() - startTime;
  Console::printf(_T("%lld ms, producerSum=%llu, consumerSum=%llu\n"), duration, (uint64_t)producerSum, (uint64_t)consumerSum);
}

int_t main(int_t argc, char_t* argv[])
{
  testQueue<LockFreeQueueSlow1<int_t> >("LockFreeQueueSlow1");
  testQueue<LockFreeQueue<int_t> >("LockFreeQueue");
  testQueue<MutexLockQueue<int_t> >("MutexLockQueue");
  testQueue<SpinLockQueue<int_t> >("SpinLockQueue");

  return 0;
}
