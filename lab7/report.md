# Lab 7 — Three Small Programs: Processes, Redirection, a Pipe

**Oberst Eduard, FAF-243** · Operating Systems (Track A: Linux) · 19 October 2026

Environment: Ubuntu 26.04.1 LTS, Linux 7.0.0-31-generic, gcc 15.2.0 (Ubuntu 15.2.0-16ubuntu1).
All work in `~/OS_26/lab7`; all five programs compile with `-Wall -Wextra` and produce no warnings.
The full session is in `lab7-transcript.log`.

---

## Part 1 — fork + exec + wait

```
$ gcc -Wall -Wextra -o run run.c && ./run
parent: my PID is 130657
child:  my PID is 130658, my parent is 130657
total 112
-rw-rw-r-- 1 capziro capziro   184 Oct 19 14:09 Makefile
-rwxrwxr-x 1 capziro capziro 16296 Oct 19 14:09 pipeline
...
-rw-rw-r-- 1 capziro capziro   980 Oct 19 14:09 zombie.c
parent: child 130658 ended with status 0
```

With `execlp("date", "date", (char *)NULL);` (1.3):

```
parent: my PID is 130679
child:  my PID is 130680, my parent is 130679
Mon Oct 19 02:09:33 PM EEST 2026
parent: child 130680 ended with status 0
```

With `execlp("nosuchprog", "nosuchprog", (char *)NULL);` (1.4):

```
parent: my PID is 130688
child:  my PID is 130689, my parent is 130688
exec: No such file or directory
parent: child 130689 ended with status 127
```

### Observations

**1. The line `perror("exec")` is in the child, right after `execlp`. Why is it printed only with `nosuchprog`?**

Because a successful `execlp` never returns. It does not call the new program — it *replaces* the
calling process: the kernel discards the old text, data, heap and stack and loads `ls` in their
place, keeping the same PID. The instruction after `execlp` belongs to a program image that no
longer exists, so there is nothing left to execute it. `execlp` returns only when the replacement
could not happen, and then it returns −1 with `errno` set. Reaching `perror("exec")` *is* the error
report: the three runs above show it exactly once, on the run where `/usr/bin/nosuchprog` does not
exist, and `perror` prints the `errno` the kernel set, `ENOENT` — "No such file or directory".
This is also why the two `exit` paths differ: `ls` and `date` end through their own `exit`, while
the failed run falls through to our `exit(127)`.

**2. Which number does the child print as "my parent", and whose PID is it?**

It prints 130657 in the first run — the same number the parent printed one line earlier with
`getpid()`. So `getppid()` in the child returns the PID of the `./run` process that called `fork`.
That is the whole point of the pair: `fork` returns twice, 0 in the child and the child's PID in
the parent, and the two processes can name each other from opposite directions —
the parent has `pid` from `fork`, the child has `getppid()`. The number is not fixed to the
program: it changed to 130679 and 130688 in the later runs, because each `./run` is a new process.

**3. What status does the parent see with `ls`, and what status with `nosuchprog`?**

`WEXITSTATUS(status)` is **0** for `ls` and **127** for `nosuchprog`. The 0 comes from `ls` itself:
it listed the directory successfully and exited 0, and that is the value `waitpid` collected. The
127 does not come from any program that ran — nothing ran. It is the literal `exit(127)` on the
line below `perror` in our own source, reached because `execlp` returned. 127 is not arbitrary:
it is the conventional "command not found" status, the same one `bash` reports for a misspelled
command, so a shell written on top of this code would already agree with the convention.

### Extra: what the two `fflush(stdout)` calls are for

Removing both `fflush` lines and sending the output through a pipe changes the result:

```
$ ./run_noflush | cat
total 116
... the whole listing ...
parent: my PID is 131355
parent: child 131357 ended with status 0
```

Two things went wrong at once. The parent's messages moved to the *end*, because `stdout` to a pipe
is block-buffered instead of line-buffered, so they sat in the buffer until `exit` flushed it —
long after `ls` had written its listing straight to the descriptor. And the child's message
disappeared completely: it was in the child's copy of the same buffer, and `execlp` threw that
buffer away with the rest of the address space before anything flushed it. The buffer is user-space
memory in the C library, not kernel state, so `fork` copies it and `exec` destroys it. On a
terminal the bug is invisible — `stdout` is line-buffered there and all three lines appear in
order — which is exactly what makes it worth knowing.

---

## Part 2 — A zombie, on purpose

`zombie.c` is `run.c` with one line added before `waitpid`:

```c
    int status;
    sleep(20);                                   /* the new line */
    waitpid(pid, &status, 0);                    /* step 3: wait for the child */
```

```
$ gcc -Wall -Wextra -o zombie zombie.c
$ ./zombie > /dev/null &
$ sleep 2; ps -o pid,ppid,stat,cmd
    PID    PPID STAT CMD
 131086  131083 S    /bin/bash part2c.sh
 131087  131086 S    ./zombie
 131089  131087 Z    [ls] <defunct>
```

The kernel's own words for the same process, and the proof that nothing of it is left:

```
$ grep -E '^(Name|State)' /proc/130874/status
Name:	ls
State:	Z (zombie)
$ cat /proc/130874/cmdline | wc -c
0
```

Twenty seconds later, the same command and the same PIDs:

```
$ ps -o pid,ppid,stat,cmd
    PID    PPID STAT CMD
 131086  131083 S    /bin/bash part2c.sh
```

### Observations

**1. Which letter does `ls` have in the STAT column, and what does it mean?**

**Z**, which `ps` spells out in the CMD column as `[ls] <defunct>` and `/proc/<pid>/status` as
`State: Z (zombie)`. It means `ls` has already run to completion and called `exit` — its memory,
its open descriptors and its kernel stack are all gone, which is why `/proc/130874/cmdline` is
0 bytes long and why `ps` has to print the name in brackets, from the last thing it remembers.
What survives is one entry in the process table holding the PID and the exit status, kept
because somebody may still ask for it. A zombie consumes no CPU and no memory, only a PID — but
PIDs are a finite resource, which is why a program that leaks zombies eventually cannot fork.

**2. After 20 seconds, is the `[ls] <defunct>` line still there? Which line of `zombie.c` made it disappear?**

No — it is gone, and so is `./zombie` itself. The line that removed it is
`waitpid(pid, &status, 0);`. That call is what the dead child was waiting for: it reads the saved
exit status into `status`, and the kernel then drops the process table entry, which is what
"reaping" means. The parent vanishes from the listing in the same instant for an unrelated reason —
it has nothing left to do after printing its last message. The experiment shows the zombie is not
a malfunction but a deliberate interval: the status has to be kept from the moment the child exits
until the moment the parent asks for it, and `sleep(20)` simply makes that interval long enough
to see with `ps`.

### Extra: what if the parent never waits at all?

```
$ ps -o pid,ppid,stat,cmd -p 130961 -p 130963
    PID    PPID STAT CMD
 130961  130862 S    ./zombie
 130963  130961 Z    [ls] <defunct>
$ kill -9 130961      # the parent dies without ever calling waitpid
$ ps -o pid,ppid,stat,cmd -p 130961 -p 130963
(neither process exists any more)
```

Killing the parent mid-sleep does not leave the zombie behind for ever: an orphan is re-parented
to `init`/`systemd` (PID 1), whose whole job includes calling `wait` in a loop, so it was reaped
immediately. A zombie therefore only lasts as long as its negligent parent does.

---

## Part 3 — Redirection: `ls -l > out.txt`

```
$ gcc -Wall -Wextra -o redirect redirect.c && ./redirect
open gave descriptor 1
done: the listing is in out.txt
$ head -3 out.txt
total 112
-rw-rw-r-- 1 capziro capziro   184 Oct 19 14:09 Makefile
-rw-r--r-- 1 capziro capziro     0 Oct 19 14:09 out.txt
$ wc -l out.txt
13 out.txt
```

Nothing from `ls` reached the terminal, and nothing reached a pipe either:

```
$ ./redirect | cat
open gave descriptor 1
done: the listing is in out.txt
```

### Observations

**1. Which number did `open` give, and why exactly that one?**

It gave **1**. `open` is specified to return the lowest-numbered descriptor not currently in use by
the process, and `close(1)` on the line above had just made 1 unused: 0 (stdin) and 2 (stderr) were
still open, so 1 was the smallest free slot. There is no connection between `open` and standard
output — the number is simply the first hole in the descriptor table, and the hole was punched
deliberately. Removing that one `close(1)` proves it:

```
$ ./redirect_noclose
open gave descriptor 3
total 116
... the listing, on the terminal ...
done: the listing is in out.txt
$ head -2 out.txt      # empty: created and truncated, never written
```

With 0, 1 and 2 all still occupied, the lowest free number is 3, `ls` keeps the terminal on its
descriptor 1, and `out.txt` is left empty — created and truncated by `O_CREAT|O_TRUNC`, but never
written to. (This is also why `out.txt` shows a size of 0 inside its own listing: `open` truncated
it before `ls` ran, and `ls` read its size before anything had been written.)

**2. `ls` was not changed at all. Why does its listing not appear on the screen?**

Because `ls` writes to descriptor 1 — it always has, and it still does — and in this child
descriptor 1 no longer refers to the terminal but to `out.txt`. The redirection happened entirely
before `ls` existed: `fork` gave the child a copy of the descriptor table, `close` and `open`
rearranged *that copy*, and `exec` replaces the program image while leaving the descriptor table
untouched, since descriptors live in the kernel, not in the address space. So `ls` inherits an
environment that was prepared for it and cannot tell the difference: there is no flag, no argument
and no code path in `ls` for redirection, which is precisely why `> out.txt` works with every
program ever compiled. Note also that it is the parent's own `printf` that still reaches the
screen — the parent never closed anything, and only the child's table was altered.

**3. To redirect the errors instead (like `2> err.txt`), which descriptor would you close before `open`?**

Descriptor **2**. Same mechanism, different hole: close 2, and 2 becomes the lowest free number for
`open` to return. Verified with a variant that closes 2, opens `err.txt`, and runs `ls /nosuchdir`
so that there is an error message to capture:

```
$ ./redirect_err
open gave descriptor 2
done: the error message is in err.txt
$ cat err.txt
ls: cannot access '/nosuchdir': No such file or directory
```

`open` returned 2, the diagnostic landed in the file, and the terminal stayed clean. Writing this
variant also reproduced the Part 1 buffering trap: a first attempt printed the descriptor number
with `printf` and no `fflush`, and the message was destroyed by `exec` — the line only appeared
after adding `fflush(stdout)`.

---

## Part 4 — A pipe: `ls /etc | wc -l`

```
$ gcc -Wall -Wextra -o pipeline pipeline.c && ./pipeline
247
$ ls /etc | wc -l
247
```

After commenting out the parent's `close(p[1]);` (4.3):

```
$ ./pipeline        # it never ends; stopped after 5 s instead of Ctrl-C
exit status: 124 (124 = killed by timeout, i.e. it was still running)
```

It prints nothing at all, and the process table shows who is left:

```
$ ps -o pid,ppid,stat,cmd
    PID    PPID STAT CMD
 131122  131120 S    ./pipeline_broken
 131125  131122 S    wc -l
pid 131125  comm=wc  state=S  wchan=anon_pipe_read
```

### Observations

**1. Did `./pipeline` and `ls /etc | wc -l` give the same number?**

Yes — **247** both times. The hand-built pipeline and the shell's own are the same construction:
one `pipe`, two `fork`s, `close`+`dup` in each child to move the right end onto the right standard
descriptor, and a parent that closes both ends and waits twice. The optional three-stage version
agrees as well: `./pipeline3` and `ls /usr/bin | grep x | wc -l` both print **243**, with two pipes,
three children, and every child closing all four pipe ends after its `dup`s.

**2. After the change, which of the two children never ends, `ls` or `wc`?**

`wc`. The table above lists exactly one surviving child, `wc -l`, and `ls` is not there at all: it
wrote its 247 lines, exited normally, and the parent's first `wait(NULL)` reaped it. The parent is
then blocked in its second `wait(NULL)` for a child that will never exit. Note also what is *not*
printed: `wc -l` emits its count only after it has finished counting, so a `wc` that never reaches
end-of-file produces no output at all, which is why the broken run prints nothing rather than a
wrong number.

**3. Why, in one sentence: what is `wc` waiting for?**

`wc` is waiting for end-of-file on its descriptor 0, and a pipe only reports end-of-file when the
*last* writing end is closed — the parent still holds `p[1]` open, so `read` has no reason to
return 0 and blocks for ever. The kernel names the wait itself: `/proc/131125/wchan` reads
`anon_pipe_read`. The pipe counts writers, not writes, and the parent is a writer that will never
write — which is the reason every process in a pipeline must close every end it does not use, and
why that duty falls on the parent as much as on the children.

---

## Conclusion

Three programs of thirty lines each answered the three questions the lecture left open, and in
every case the answer turned out to be that the shell has no special powers at all.

A process is born by `fork`, which returns twice, and becomes a different program by `exec`, which
returns once — only on failure. That asymmetry is the whole explanation of Part 1: `perror("exec")`
appeared only for `nosuchprog`, because in the two successful runs there was no program left to
execute that line, and the status 127 the parent saw came from our own `exit(127)` rather than from
anything that ran. `wait` is the third step and not an optional one: `sleep(20)` before `waitpid`
held the finished `ls` in state `Z` long enough for `ps` to show it with no memory and an empty
`cmdline`, and the one line `waitpid(pid, &status, 0)` made it disappear. Killing the parent instead
showed the safety net — `init` adopts the orphan and reaps it at once — so a zombie is a measurable
interval, not a leak, unless its parent lives on without ever asking.

Redirection and pipes then proved to be the same trick applied twice, and the trick is that
descriptor numbers are allocated lowest-free-first while `exec` leaves the descriptor table alone.
In `redirect.c` the `close(1)` punched the hole that made `open` return 1 — with the `close`
removed, the identical `open` returned 3 and the listing went back to the terminal — and `ls`,
unchanged and unaware, wrote to `out.txt` because writing to descriptor 1 is all it ever did.
Closing 2 instead captured the error stream, which is the entire difference between `>` and `2>`.
`pipeline.c` used the same `close`-then-`dup` on both sides of one pipe and reproduced the shell's
247 exactly, and `pipeline3.c` scaled it to two pipes and 243. The deliberate bug was the most
instructive part: leaving the parent's `p[1]` open left `wc` sleeping in `anon_pipe_read` for ever,
because a pipe signals end-of-file by counting writers rather than writes, and the parent was a
writer that never intended to write.

The through-line of the lab is that `fork`, `exec`, `wait`, `close`, `open`, `pipe` and `dup` are
not machinery hidden under the shell — they are the shell, and `ls -l > out.txt | wc -l` is a
short program written in them. The one thing that genuinely does not survive `fork` and `exec` is
the C library's own output buffer, which is user-space memory: without `fflush`, the parent's
messages were reordered to the end of a piped run and the child's were destroyed outright. On a
terminal that bug is invisible, which is the best argument for having looked at the pipe.
