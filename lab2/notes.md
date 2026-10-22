# Lab 2 Notes

## Part 2

1. Three programs included in xv6 are:
   - ls
   - cat
   - echo

2. For a pipe to work, the operating system must support processes and
   inter-process communication.

3. The xv6 shell is similar to the Linux shell because it supports basic
   commands, processes, pipes and file operations, but it has fewer features.
## Part 3

1. user/cat.c uses system calls such as open(), read(), write(),
   close(), and exit().

   open() opens a file.
   read() reads data from a file descriptor.
   write() writes data to a file descriptor.
   close() closes a file descriptor.
   exit() terminates the process.

2. sys_read is implemented in kernel/sysfile.c.
   The function name is on line 69 (its uint64 return type is on line 68).

3. kernel/ contains privileged operating-system code, while user/
   contains normal programs that use system calls to request services
   from the kernel.

## Ubuntu setup and API compatibility

I use Ubuntu directly as the host, so no separate Linux VM is required.
QEMU emulates the RISC-V hardware on which xv6 runs.

The checked-out xv6 revision is 06aad25. In this revision the tick-delay
system call is named pause(), declared in user/user.h and implemented by
sys_pause() in kernel/sysproc.c. user/sleep.c uses a small sleep() wrapper
that calls pause(ticks), preserving the lab program name and behavior.
The Makefile UPROGS list includes $U/_sleep so the program enters fs.img.

## Verification

- make qemu compiled user/sleep.c, rebuilt fs.img, and booted to the xv6 shell.
- ls listed sleep alongside the standard user programs.
- cat README displayed the file and echo hello xv6 printed hello xv6.
- ls | grep c filtered the directory listing through a pipe.
- wc README returned 48 lines, 336 words, and 2441 bytes.
- sleep 10 paused and returned to the shell without output.
- sleep and sleep 10 20 both printed usage: sleep <ticks>.
- QEMU was closed with Ctrl-A followed by X.
