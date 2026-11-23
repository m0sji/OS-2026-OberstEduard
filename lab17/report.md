# Lab 17 – Threads Share Memory, and Race

Oberst Eduard, FAF-243

## Part 1. Shared and racing

1. It proves that the two threads use the same memory. `counter` is one global variable, and thread 2 read the value that thread 1 had already written, for example 7568 or 44613, without any copy or message between them. In my 20 runs thread 2 started with a non-zero value 13 times. In the other 7 runs it printed 0, because on my 16-core laptop both threads start almost at the same moment and thread 2 read `counter` before thread 1 began its loop.

2. `counter++` is three steps. The thread loads the value from memory into a register, adds 1 in the register and stores it back. Two threads on two cores can both load the same value, for example 500, both add 1 and both store 501. Two increments were done but the counter grew by one, so one update is lost. How often this overlap happens depends on the timing of the threads on the cores, which is different every run. In my 10 runs the counter was between 1000186 and 1568207 instead of 2000000, a loss from 21.6% to 50.0%.

## Part 2. Make it worse, then think

With four threads the loss got bigger. Over 10 runs each, two threads lost 45.5% on average and four threads lost 69.2% of 4000000. In another 10 listed runs of each program the four-thread loss went up to 74.0%, and the final counter was 1302674 on average, only about 256000 more than the 1046418 reached by two threads.

The fix I propose on paper is to put `counter++` between `pthread_mutex_lock(&lock)` and `pthread_mutex_unlock(&lock)`, so only one thread at a time can do the three steps.

3. Mutual exclusion, which means making the read, add and write of `counter++` happen as one step that only one thread at a time can enter, for example with a mutex around it.
