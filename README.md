LockFreeQueue
=============

Here, I am testing some multi-producer multi-consumer bounded ring buffer FIFO queue implementations for fun.

* [LockFreeQueueCpp11.h](LockFreeQueueCpp11.h) - The fastest lock free queue I have managed to implement.
* [LockFreeQueue.h](LockFreeQueue.h) - The fastest lock free queue I have managed to implement without c++11. It is equally fast as LockFreeQueueCpp11.h.
* [mpmc_bounded_queue.h](mpmc_bounded_queue.h) - Bounded MPMC queue by [Dmitry Vyukov, 2011]
* [LockFreeQueueSlow1.h](LockFreeQueueSlow1.h) - My first attempt at implementing a lock free queue. It is working correctly, but it is a lot slower than LockFreeQueue.h.
* [LockFreeQueueSlow2.h](LockFreeQueueSlow2.h) - A lock free queue based on [John D. Valois, 1994]. The queue uses Valois' algorithm adapted to a ring buffer structure with some modifications to tackle the ABA-Problem.
* [LockFreeQueueSlow3.h](LockFreeQueueSlow3.h) - Another lock free queue almost as fast as LockFreeQueue.h.
* [MutexLockQueue.h](MutexLockQueue.h) - A naive queue implementation that uses a conventional mutex lock (CriticalSecion / pthread-Mutex).
* [SpinLockQueue.h](SpinLockQueue.h) - A naive queue implementation that uses an atomic TestAndSet-lock.

And for the fun of it, here is a multi-producer multi-consumer LIFO queue:

* [LockFreeLifoQueue.h](LockFreeLifoQueue.h) - A lock free multi-producer multi-consumer bounded LIFO queue.

#### References

[John D. Valois, 1994] - Implementing Lock-Free Queues<br/>
[Dmitry Vyukov, 2011] - [Bounded MPMC queue](http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)