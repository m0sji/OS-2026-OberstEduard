# Lab 8 — The Same Calls, Inside Your Own OS (xv6)

**Oberst Eduard, FAF-243** · Operating Systems (Track B: xv6) · 19 October 2026

Tree: `~/xv6-riscv` at commit `06aad25`, built with `riscv64-linux-gnu-gcc 15.2.0`,
run under QEMU 10.2.1 on Ubuntu 26.04.1 LTS. Three programs added — `user/run.c`,
`user/pingpong.c`, `user/primes.c` — each with its `$U/_NAME\` line in `UPROGS`.
Full session in `lab8-transcript.log`.

---

## Part 1 — `run`, inside xv6

```
$ run
parent: my pid is 3
child: my pid is 4
hello from the child
parent: child 4 ended with status 0
```

With `char *args[] = { "ls", 0 }; exec("ls", args);` (1.3):

```
$ run
parent: my pid is 3
child: my pid is 4
.              1 1 1024
..             1 1 1024
README         2 2 2441
...
run            2 27 35888
pingpong       2 28 35968
primes         2 29 36456
console        3 30 0
parent: child 4 ended with status 0
```

With `exec("nosuchprog", args);` (1.4):

```
$ run
parent: my pid is 3
child: my pid is 4
exec failed
parent: child 4 ended with status 1
```

### Observations

**1. Write the three calls that `run.c` uses in xv6 and the three calls `run.c` used on Linux in Lab 7. What is the same?**

| step | xv6 (Lab 8) | Linux (Lab 7) |
|---|---|---|
| 1. copy myself | `fork()` | `fork()` |
| 2. become another program | `exec("echo", args)` | `execlp("ls", "ls", "-l", NULL)` |
| 3. wait for the child | `wait(&status)` | `waitpid(pid, &status, 0)` |

What is the same is everything that matters — the *shape* of the thing. Both programs are the
same three steps in the same order, and the properties of each step are identical: `fork` returns
twice, 0 in the child and the child's PID in the parent; `exec` replaces the program image and
therefore returns only when it has failed; `wait` blocks until the child is finished and hands
back its exit status. The behaviour I measured in Lab 7 on Linux reappeared unchanged here on a
kernel of a few thousand lines that I compiled myself, which is the point of the exercise: these
are not glibc conveniences layered on top of an OS, they are the interface of the OS.

What differs is only spelling and ambition. xv6's `exec` takes the argument vector as an array,
while `execlp` is variadic and additionally searches `PATH` — xv6 has no `PATH`, so `exec("echo", …)`
is a path, which is why it works only because `echo` sits in the root directory. xv6's `wait` takes
just a status pointer and reaps *any* child, whereas `waitpid` can name one particular child and
takes a flags argument. And the status itself is plain in xv6 — `wait(&status)` leaves the exit
code in `status` directly — while Linux packs it into a word that has to be unpacked with
`WEXITSTATUS`. There is also no `perror` and no `errno` here: the failure is reported by our own
`printf("exec failed\n")`, because this kernel has nothing else to offer.

**2. Who prints "hello from the child": the code of `run`, or `echo`?**

`echo` does. The string never appears in `run.c` at all — what `run.c` contains is the argument
vector `{ "echo", "hello", "from", "the", "child", 0 }`, five separate words and a terminating 0,
and `user/echo.c` is what turns them into a line:

```c
for (i = 1; i < argc; i++) {
  write(1, argv[i], strlen(argv[i]));
  if (i + 1 < argc) write(1, " ", 1); else write(1, "\n", 1);
}
```

So the spaces between the words and the newline at the end are `echo`'s doing, not ours, and they
are written with `write(1, …)` — descriptor 1, exactly as in Lab 7. The replacement in step 1.3
proves the same point from the other side: changing two lines to `exec("ls", args)` made the
directory listing appear, and that listing is obviously the work of `ls` and not of `run`. After
`exec` succeeds there is no code of `run` left running in that process at all — only its PID, its
parent and its open descriptors survive.

**3. Why is the status 1 here and 127 on Linux? (Look at the `exit` line after `exec` in both programs.)**

Because in both labs the status after a failed `exec` is produced by *our own source*, and the two
sources were written with different numbers. In this program the line after `exec` is `exit(1)`, so
the parent sees 1; in the Lab 7 program the line after `execlp` was `exit(127)`, so the parent saw
127. The kernel chose neither value. The reason a number of ours gets through at all is the
property from question 1: `exec` returns only on failure, so the `printf("exec failed\n")` and the
`exit(1)` below it are reached precisely when there is no new program to override them — and
indeed `exec failed` is printed in the 1.4 run and in no other.

127 is not a better answer than 1, only a conventional one: it is the "command not found" status
that Unix shells report, so a Linux program that intends to behave like a shell picks it
deliberately. xv6's own shell does not implement that convention, so 1 — the generic "something
went wrong" — is the natural choice here. The comparison is a good reminder that an exit status is
just a byte the program chooses, and that any meaning it carries is a convention between programs
rather than a rule of the kernel.

---

## Part 2 — `pingpong`

```
$ pingpong
6: received ping
5: received pong
```

"ping" comes first and from the higher PID (the child, 6); "pong" second, from the parent (5).

### Observations

**1. Why do you need TWO pipes, and not one for both directions?**

Because a pipe is one-directional: it has a read end and a write end, and the bytes only ever
travel from the second to the first. One pipe cannot carry the ping in one direction and the pong
back in the other, because both processes would have to read and write the *same* channel — and
nothing would stop a process from reading the byte it has just written itself. That is exactly
what happens. A one-pipe version, identical except that `p[0]` and `p[1]` are used for both
directions, prints this and then hangs:

```
$ pp1pipe
7: received pong
(no prompt: the program never ends)
```

Only one line appeared, and it is the *parent's*. The parent wrote its byte, immediately read
from the same pipe, and got its own byte back — so it announced "pong" without any pong ever
having been sent. The child, meanwhile, never received anything and is still blocked in `read`,
which is why the parent's `wait(0)` never returns and xv6 never printed a new prompt. With two
pipes this is impossible by construction: each process closes the two ends it must not touch, so
`p2c` can only carry bytes parent → child and `c2p` only child → parent, and a byte written into
one can never be read back by its own writer.

**2. Nothing in your code says "wait until the child has replied". Which call makes the parent wait?**

`read(c2p[0], &b, 1)`. A `read` on a pipe with no data in it blocks until either a byte arrives or
every writing end is closed — so the parent stops on that line and does not continue to its
`printf` until the child has actually written the byte back. That single blocking call is the
entire synchronisation; the ordering of the two lines of output is a consequence of it.

It is specifically *not* `wait(0)` that orders them. `wait` only reaps the finished child, and it
runs after both messages have been printed. Removing it entirely leaves the order untouched:

```
$ ppnowait
4: received ping
3: received pong
$ ppnowait
6: received ping
5: received pong
```

"ping" still precedes "pong" in both runs, because the thing that holds the parent back is the
`read`, not the `wait`. The same applies in the child, which begins by blocking in
`read(p2c[0], &b, 1)` and so cannot print "ping" before the parent has sent anything — the two
blocking reads between them are what make the conversation strictly alternate.

---

## Optional — `primes`

```
$ primes
prime 2
prime 3
prime 5
prime 7
prime 11
prime 13
prime 17
prime 19
prime 23
prime 29
prime 31
```

All eleven primes below 35, each printed by a different process. Two things were worth getting
right. The chain is built with a **loop rather than recursion**: a first attempt called the stage
function recursively in the child and was rejected by the compiler with
`error: infinite recursion detected [-Werror=infinite-recursion]`, since nothing in the source
tells gcc that the recursive call only ever happens in a forked child. Letting the child simply
take over the loop with its own left-hand descriptor removes the problem and costs no stack. And
every stage must **close both ends it does not use** plus its left end when it is done — the final
`close(right[1])` is what gives the next stage end-of-file and lets the chain unwind; without it
the pipeline would hang exactly like the broken `pipeline.c` of Lab 7, for exactly the same
reason.

---

## Conclusion

The point of this lab was that nothing new had to be learned. The program of Lab 7 was rewritten
for a kernel of a few thousand lines that I compiled myself, and `fork`, `exec` and `wait` behaved
precisely as they had on Linux: `fork` returned twice, `exec` returned only when it failed — which
is why `exec failed` appeared in the 1.4 run and nowhere else — and `wait` handed back a status
that our own `exit` line had chosen, 1 here and 127 in Lab 7. `echo` printed "hello from the
child", not `run`, because after a successful `exec` there is no `run` left in that process.

`pingpong` then showed the pipe from the other side of the system call. Two pipes are needed
because a pipe is one-directional, and the one-pipe version demonstrated the failure concretely
rather than theoretically: the parent read back its own byte, announced a pong nobody sent, and
hung waiting for a child still blocked in `read`. The call that synchronises the two processes is
`read`, not `wait` — removing `wait(0)` left the ordering intact. `primes` extended the same idea
to a chain of eleven processes, where the rule from Lab 7 reappeared unchanged: close every end
you do not use, or the chain never ends.
