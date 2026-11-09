// trytest.c: sem_trydown never sleeps, it answers 0 or -1 at once.
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  sem_init(4, 1);
  printf("first try:  %d\n", sem_trydown(4));   // 0: the value was 1
  printf("second try: %d\n", sem_trydown(4));   // -1: the value is 0, no sleep
  sem_up(4);
  printf("after up:   %d\n", sem_trydown(4));   // 0 again
  exit(0);
}
