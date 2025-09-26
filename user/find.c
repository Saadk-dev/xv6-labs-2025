// user/find.c — find all files named <target> under <start-path>
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"     // struct dirent, DIRSIZ, T_DIR, T_FILE
#include "user/user.h"
#include "kernel/fcntl.h"

static void find(const char *path, const char *target) {
  struct stat st;

  if (stat(path, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    return;
  }

  // If start path is a *file*, compare its basename and print if it matches.
  if (st.type == T_FILE) {
    // compute basename(path)
    const char *b = path + strlen(path);
    while (b > path && *(b - 1) != '/') b--;
    if (strcmp(b, target) == 0)
      printf("%s\n", path);
    return;
  }

  if (st.type != T_DIR) {
    // Not a file and not a directory; nothing to do.
    return;
  }

  // Directory: iterate entries and recurse.
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  char buf[512];
  struct dirent de;

  // base path in buf, and pointer p after it
  strcpy(buf, path);
  char *p = buf + strlen(buf);
  if (p == buf || *(p - 1) != '/')
    *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) continue;

    // de.name isn’t guaranteed NUL-terminated; make a proper C string
    char name[DIRSIZ + 1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    // build path = <path>/<name>
    char *q = p;
    memmove(q, name, strlen(name));
    q[strlen(name)] = 0;

    if (stat(buf, &st) < 0) {
      // skip unreadable children
      continue;
    }

    if (st.type == T_DIR) {
      // recurse into subdirectory
      find(buf, target);
    } else if (st.type == T_FILE) {
      if (strcmp(name, target) == 0)
        printf("%s\n", buf);
    }

    // restore buf back to "<path>/"
    *p = 0;
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "usage: find <start-path> <filename>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
