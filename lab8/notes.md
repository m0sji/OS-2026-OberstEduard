# Lab 8 – notes (answers to the "Observe" boxes)

> Note: the experiments, commands and screenshots were done by me on my own laptop (see `lab8-transcript.log`).
> The four TODO lines of `pingpong.c` and the answers below were written with the help of the AI assistant Claude,
> based on my results.

## Part 1. run, inside xv6

Outputs (from `lab8-transcript.log`):

```
$ run                                   (echo version)
parent: my pid is 3
child: my pid is 4
hello from the child
parent: child 4 ended with status 0

$ run                                   (exec("nosuchprog", args))
parent: my pid is 3
child: my pid is 4
exec failed
parent: child 4 ended with status 1
```

1. **The three calls in xv6 and on Linux. What is the same?**
   xv6: `fork()`, `exec("echo", args)`, `wait(&status)`. Linux (Lab 7): `fork()`, `execlp("ls", "ls", "-l", NULL)`,
   `waitpid(pid, &status, 0)`. The same three steps in the same order, and they behave the same: `fork` returns twice
   (parent pid 3, child pid 4), `exec` replaces the child's program and comes back only if it fails, and `wait` blocks
   until the child ends and gives its exit status. Only details differ: the xv6 `exec` takes an array and does not
   search a PATH, and the xv6 `wait` waits for any child.

2. **Who prints "hello from the child"?**
   `echo`. `run.c` only passes the words as arguments to `exec`; after `exec` the child is the `echo` program, which
   prints its arguments. In step 1.3 the same child printed the `ls` listing instead.

3. **Why is the status 1 here and 127 on Linux?**
   Because the number comes from our own code: when `exec` fails, the child goes on to the next line, which is
   `exit(1)` in the xv6 `run.c` and was `exit(127)` in the Linux `run.c` of Lab 7.

## Part 2. pingpong

```
$ pingpong
6: received ping
5: received pong
```

1. **Why two pipes and not one?**
   A pipe carries data in one direction only, from the write end to the read end. With one pipe for both directions
   the parent could read back its own byte right after writing it, and the child would never get it. With two pipes
   `p2c` goes only parent → child and `c2p` only child → parent, and each process closes the ends it does not use.

2. **Which call makes the parent wait for the reply?**
   `read(c2p[0], &b, 1)`: a `read` on an empty pipe blocks until a byte arrives, so the parent waits there until the
   child writes the byte back. That is why "ping" (child, pid 6) comes before "pong" (parent, pid 5). It is not
   `wait(0)`, which is called only after the pong is printed.
