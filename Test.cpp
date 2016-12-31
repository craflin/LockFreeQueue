
#include <nstd/Console.h>
#include <nstd/Debug.h>
#include <nstd/Thread.h>
#include <nstd/List.h>
#include <nstd/Time.h>

#include "LockFreeQueue.h"
#include "LockFreeQueueSlow1.h"
#include "MutexLockQueue.h"
#include "SpinLockQueue.h"

static const int testItems = 250000 * 64 / 3;
static const int testThreadConsumerThreads = 8;
static const int testThreadProducerThreads = 8;
static const int testItemsPerConsumerThread = testItems / testThreadConsumerThreads;
static const int testItemsPerProducerThread = testItems / testThreadProducerThreads;

template<typename T> class IQueue
{
public:
  virtual usize size() const = 0;
  virtual bool push(const T& data) = 0;
  virtual bool pop(T& result) = 0;
};

template<typename T, class Q> class TestQueue : public IQueue<T>
{
public:
  TestQueue(usize size) : queue(size) {}
  usize size() const {return queue.size();}
  bool push(const T& data) {return queue.push(data);}
  bool pop(T& result) {return queue.pop(result);}
private:
  Q queue;
};

volatile size_t producerSum;
volatile size_t consumerSum;
volatile int64 maxPushDuration;
volatile int64 maxPopDuration;

uint producerThread(void* param)
{
  IQueue<int>* queue = (IQueue<int>*)param;
  for(int i = 0; i < testItemsPerProducerThread; ++i)
  {
    for(;;)
    {
      int64 startTime = Time::microTicks();
      while(!queue->push(i))
      {
        Thread::yield();
        startTime = Time::microTicks();
      }
      {
        int64 duration = Time::microTicks() - startTime;
        for(;;)
        {
          int64 lmaxPushDuration = maxPushDuration;
          if(duration <= lmaxPushDuration || Atomic::compareAndSwap(maxPushDuration, lmaxPushDuration, duration) == lmaxPushDuration)
            break;
        }
        break;
      }
    }
    Atomic::fetchAndAdd(producerSum, i);
  }
  return 0;
}

uint consumerThread(void* param)
{
  IQueue<int>* queue = (IQueue<int>*)param;
  int val;
  for(int i = 0; i < testItemsPerConsumerThread; ++i)
  {
    for(;;)
    {
      int64 startTime = Time::microTicks();
      while(!queue->pop(val))
      {
        Thread::yield();
        startTime = Time::microTicks();
      }
      {
        int64 duration = Time::microTicks() - startTime;
        for(;;)
        {
          int64 lmaxPopDuration = maxPopDuration;
          if(duration <= lmaxPopDuration || Atomic::compareAndSwap(maxPopDuration, lmaxPopDuration, duration) == lmaxPopDuration)
            break;
        }
        break;
      }
    }
    Atomic::fetchAndAdd(consumerSum, val);
  }
  return 0;
}

template<class Q> void testQueue(const String& name)
{
  Console::printf(_T("Testing %s... \n"), (const tchar*)name);

  volatile int32 int32_ = 0;
  volatile uint32 uint32_ = 0;
  ASSERT(Atomic::compareAndSwap(int32_, 0, 1) == 0);
  ASSERT(int32_ == 1);
  ASSERT(Atomic::compareAndSwap(uint32_, 0, 1) == 0);
  ASSERT(uint32_ == 1);

  volatile int64 int64_ = 0;
  volatile uint64 uint64_ = 0;
  ASSERT(Atomic::compareAndSwap(int64_, 0, 1) == 0);
  ASSERT(int64_ == 1);
  ASSERT(Atomic::compareAndSwap(uint64_, 0, 1) == 0);
  ASSERT(uint64_ == 1);

  {
    TestQueue<int, Q> queue(10000);
    int result;
    ASSERT(!queue.pop(result));
    ASSERT(queue.push(42));
    ASSERT(queue.pop(result));
    ASSERT(result == 42);
    ASSERT(!queue.pop(result));
  }

  //{
  //  TestQueue<int, Q> queue(0);
  //  int result;
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //  ASSERT(!queue.pop(result));
  //  ASSERT(!queue.push(42));
  //}

  {
    TestQueue<int, Q> queue(2);
    int result;
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
  maxPushDuration = 0;
  maxPopDuration = 0;

  int64 microStartTime = Time::microTicks();
  {
    TestQueue<int, Q> queue(100);
    List<Thread*> threads;
    for(int i = 0; i < testThreadProducerThreads; ++i)
    {
      Thread* thread = new Thread;
      thread->start(producerThread, &queue);
      threads.append(thread);
    }
    for(int i = 0; i < testThreadConsumerThreads; ++i)
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
    ASSERT(producerSum == consumerSum);
  }
  int64 microDuration = Time::microTicks() - microStartTime;
  Console::printf(_T("%lld ms, maxPush: %lld microseconds, maxPop: %lld microseconds\n"), microDuration / 1000, maxPushDuration, maxPopDuration);
}

int main(int argc, char* argv[])
{
  for(int i = 0; i < 3; ++i)
  {
    Console::printf(_T("--- Run %d ---\n"), i);
    testQueue<LockFreeQueueSlow1<int> >("LockFreeQueueSlow1");
    testQueue<LockFreeQueue<int> >("LockFreeQueue");
    testQueue<MutexLockQueue<int> >("MutexLockQueue");
    testQueue<SpinLockQueue<int> >("SpinLockQueue");
  }

  return 0;
}
