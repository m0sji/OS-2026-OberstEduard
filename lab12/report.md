# Lab 12 – Producer, Consumer and the Other Tools

Oberst Eduard, FAF-243

## Part 1. Producer and consumer

1. The call `sem_wait(&empty)` in `producer()` stops it. `empty` starts at 4 and every put takes one unit, so after 4 puts the producer waits until the consumer calls `sem_post(&empty)`.
2. The consumer waits at the start. `full` starts at 0, so it stops at `sem_wait(&full)` until the producer puts the first item and calls `sem_post(&full)`.
3. The program hangs. In my run it printed put 1 to put 4 and then `timeout 5` killed it with exit code 124. When the buffer is full, the producer holds the mutex and waits in `sem_wait(&empty)`. The consumer is the only thread that can free a slot, but it needs the same mutex first, so it waits in `pthread_mutex_lock`. Each thread waits for the other one, which is a deadlock. The semaphore must be taken before the mutex.

## Part 2. The condition variable

1. The variable `ready`, protected by the mutex `lock`. The condition variable remembers nothing. A signal sent when nobody waits is lost.
2. Yes, it still works here. There is only one reader and one writer, and A sets `ready = 1` before it signals, so B finds the message written when it wakes up.
3. A thread can return from `pthread_cond_wait` without a signal, which is called a spurious wakeup and is allowed by POSIX. Also, with several waiters another thread can wake up first and change the state before this thread gets the mutex. With `while` the thread checks `ready` again after every wakeup.

## Part 3. Readers and writers

1. `if (++readers == 1) sem_wait(&room);` and `if (--readers == 0) sem_post(&room);`. Only the first reader takes the room and only the last one gives it back, so the other readers enter freely while a writer has to wait until no reader is inside.
2. `++readers` and `--readers` are load, add and store, like `counter++` in Lab 11. Without `rlock` two readers can both read 0 and write 1. Both then think they are first and both call `sem_wait(&room)`, so the second one blocks forever. The counter can also go wrong when readers leave, so the room is never given back or is given back while a reader is still inside.
3. Writer 2 waits forever, because `readers` never goes down to 0 and `sem_post(&room)` is never called. This is starvation.

## Part 4. Deadlock

1. Thread one holds A and waits for B. Thread two holds B and waits for A.

```
one --waits for--> B --held by--> two
 ^                                 |
 +--held by--- A <--waits for------+
```

2. No, it does not always hang. On my laptop it hung in 9 of 10 runs and in 82 of 100 runs. Without `sleep`, thread one can sometimes take both A and B before two starts. This is worse because the bug is still there but appears only sometimes, so it can pass tests and is hard to repeat and find.
3. If every thread takes A before B, a thread waiting for B already holds A, so no other thread can hold B while waiting for A, and the circle can never close.
