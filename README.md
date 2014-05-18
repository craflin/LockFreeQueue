LockFreeQueue
=============

Here I am testing some multi-producer multi-consumer ring buffer FIFO queue implementations for fun.

LockFreeQueue.h - A lock free queue based on [John D. Valois, 1994]. The queue uses Valois' algorithm adapted to on a ring buffer structure with some modifications to tackle the ABA-Problem.
LockFreeQueueSlow1.h - My first attempt at implementing a lock free queue. Its working correctly, but it is a lot slower than LockFreeQueue.h.
MutexLockQueue.h - A naive queue implementation that uses a conventional mutex lock (CriticalSecion / pthread-Mutex).
SpinLockQueue.h - A naive queue implementation that uses an atomic TestAndSet-lock.

#### References

[John D. Valois, 1994] - Implementing Lock-Free Queues
