#include "kernel/types.h"
#include "user/user.h"

// This xv6 version names the tick-delay system call pause().
static int
sleep(int ticks)
{
  return pause(ticks);
}

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }

  sleep(atoi(argv[1]));
  exit(0);
}
