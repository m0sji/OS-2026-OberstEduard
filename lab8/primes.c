#include "kernel/types.h"
#include "user/user.h"

// the primes from 2 to 35, one process per prime (Doug McIlroy's sieve):
// each stage reads its first number p, prints it, and forwards to the next
// stage only the numbers that p does not divide.
//
// the chain is built with a loop and not with recursion: a child that is to
// become the next stage just takes over the loop with its own left end.

#define LAST 35

int
main(int argc, char *argv[])
{
  int p[2];

  pipe(p);
  if(fork() != 0){                     // the feeder: 2, 3, 4, ... LAST
    close(p[0]);
    for(int i = 2; i <= LAST; i++)
      write(p[1], &i, sizeof(i));
    close(p[1]);                       // EOF for the first stage
    wait(0);
    exit(0);
  }

  close(p[1]);
  int left = p[0];                     // every stage reads from here

  for(;;){
    int first, n, right[2];

    if(read(left, &first, sizeof(first)) != sizeof(first)){
      close(left);                     // no numbers left: the chain ends here
      exit(0);
    }
    printf("prime %d\n", first);

    pipe(right);
    if(fork() == 0){                   // the child becomes the next stage
      close(right[1]);
      close(left);
      left = right[0];
      continue;                        // same loop, one pipe further down
    }

    close(right[0]);                   // this stage only writes to the right
    while(read(left, &n, sizeof(n)) == sizeof(n))
      if(n % first != 0)
        write(right[1], &n, sizeof(n));

    close(left);
    close(right[1]);                   // this is what gives the next stage EOF
    wait(0);
    exit(0);
  }
}
