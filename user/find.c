// user/find.c — find files named <target> under <start>, with optional -exec
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"      // struct dirent, DIRSIZ, T_DIR, T_FILE
#include "kernel/param.h"   // MAXARG
#include "kernel/fcntl.h"
#include "user/user.h"

static int exec_enabled = 0;
static int basec = 0;
static char *basev[MAXARG];   // command words after -exec

static void run_exec(const char *filepath) {
  if (!exec_enabled) return;

  // Build argv = basev ... filepath NULL
  char *argv[MAXARG];
  int k = 0;
  for (int i = 0; i < basec && k < MAXARG-1; i++) argv[k++] = basev[i];
  if (k >= MAXARG-1) {
    fprintf(2, "find: exec argv too long\n");
    return;
  }
  argv[k++] = (char*)filepath;
  argv[k]   = 0;

  int pid = fork();
  if (pid == 0) {
    exec(argv[0], argv);
    // If exec returns, it failed
    fprintf(2, "find: exec %s failed\n", argv[0]);
    exit(1);
  } else if (pid > 0) {
    wait(0);
  } else {
    fprintf(2, "find: fork failed\n");
  }
}

static void find(const char *path, const char *target) {
  struct stat st;
  if (stat(path, &st) < 0) {
    // unreadable; skip
    return;
  }

  if (st.type == T_FILE) {
    // compare basename(path) to target
    const char *b = path + strlen(path);
    while (b > path && *(b-1) != '/') b--;
    if (strcmp(b, target) == 0) {
      if (exec_enabled) {
        run_exec(path);
      } else {
        printf("%s\n", path);
      }
    }
    return;
  }

  if (st.type != T_DIR) return;

  int fd = open(path, O_RDONLY);
  if (fd < 0) return;

  char buf[512];
  struct dirent de;

  // prepare "<path>/"
  strcpy(buf, path);
  char *p = buf + strlen(buf);
  if (p == buf || *(p-1) != '/') *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) continue;

    char name[DIRSIZ+1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

    // build child path into buf
    char *q = p;
    int nlen = strlen(name);
    memmove(q, name, nlen);
    q[nlen] = 0;

    if (stat(buf, &st) < 0) { *p = 0; continue; }

    if (st.type == T_DIR) {
      find(buf, target);
    } else if (st.type == T_FILE) {
      if (strcmp(name, target) == 0) {
        if (exec_enabled) run_exec(buf);
        else printf("%s\n", buf);
      }
    }

    *p = 0; // restore "<path>/"
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    fprintf(2, "usage: find <start-path> <filename> [-exec <cmd> [args...]]\n");
    exit(1);
  }

  // detect -exec (must come after <filename>)
  exec_enabled = 0;
  basec = 0;
  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-exec") == 0) {
      exec_enabled = 1;
      // all remaining args are the command words
      for (int j = i+1; j < argc && basec < MAXARG-1; j++) basev[basec++] = argv[j];
      if (basec == 0) {
        fprintf(2, "find: -exec needs a command\n");
        exit(1);
      }
      break;
    }
  }

  find(argv[1], argv[2]);
  exit(0);
}
