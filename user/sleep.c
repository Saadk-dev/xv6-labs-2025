#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int valid(const char *s){ if(*s==0) return 0; for(const char*p=s;*p;p++) if(*p<'0'||*p>'9') return 0; return 1; }

int main(int argc, char *argv[]) {
  if (argc != 2) { fprintf(2, "usage: sleep ticks\n"); exit(1); }
  if (!valid(argv[1])) { fprintf(2, "sleep: ticks must be a non-negative integer\n"); exit(1); }
  int n = atoi(argv[1]);
  // your fork exposes pause(int ticks)
  if (pause(n) < 0) { fprintf(2, "sleep: pause(%d) failed\n", n); exit(1); }
  exit(0);
}
