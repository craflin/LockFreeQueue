LockFreeQueue
=============

Here I am testing some multi-producer multi-consumer ring buffer FIFO queue implementations for fun.

* [LockFreeQueue.h](LockFreeQueue.h) - The fastest lock free queue I have tested so far.
* [LockFreeQueueSlow1.h](LockFreeQueueSlow1.h) - My first attempt at implementing a lock free queue. Its working correctly, but it is a lot slower than LockFreeQueue.h.
* [LockFreeQueueSlow2.h](LockFreeQueueSlow2.h) - A lock free queue based on [John D. Valois, 1994]. The queue uses Valois' algorithm adapted to a ring buffer structure with some modifications to tackle the ABA-Problem.
* [LockFreeQueueSlow3.h](LockFreeQueueSlow3.h) - Another lock free queue almost as fast as LockFreeQueue.h.
* [MutexLockQueue.h](MutexLockQueue.h) - A naive queue implementation that uses a conventional mutex lock (CriticalSecion / pthread-Mutex).
* [SpinLockQueue.h](SpinLockQueue.h) - A naive queue implementation that uses an atomic TestAndSet-lock.

#### References

[John D. Valois, 1994] - Implementing Lock-Free Queues
