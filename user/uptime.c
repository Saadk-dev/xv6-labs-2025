#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int t = uptime();     // ticks since boot
  printf("%d\n", t);
  exit(0);
}
