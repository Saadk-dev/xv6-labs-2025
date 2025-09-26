#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"   // O_RDONLY


// separators per spec
static const char *SEPS = " -\r\t\n./,";

// Print all numbers in fd that are multiples of 5 or 6.
static void process_fd(int fd) {
  char tok[64];
  int tlen = 0;
  int digits_only = 1;

  char c;
  while (read(fd, &c, 1) == 1) {
    if (strchr(SEPS, c)) {
      if (tlen > 0 && digits_only) {
        tok[tlen] = 0;
        int n = atoi(tok);
        if (n % 5 == 0 || n % 6 == 0) {
          printf("%d\n", n);
        }
      }
      tlen = 0;
      digits_only = 1;
    } else {
      if (tlen < (int)sizeof(tok) - 1) tok[tlen++] = c;
      if (c < '0' || c > '9') digits_only = 0;
    }
  }
  // implicit separator at EOF
  if (tlen > 0 && digits_only) {
    tok[tlen] = 0;
    int n = atoi(tok);
    if (n % 5 == 0 || n % 6 == 0) {
      printf("%d\n", n);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "usage: sixfive file1 [file2 ...]\n");
    exit(1);
  }
  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      continue;
    }
    process_fd(fd);
    close(fd);
  }
  exit(0);
}
