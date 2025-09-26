// user/find.c — regex name match + optional -exec
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "user/user.h"

/* --- tiny regex like grep.c: supports ^ $ . * --- */
static int matchhere(char *re, char *text);
static int matchstar(int c, char *re, char *text);

static int match(char *re, char *text) {
  if (re[0] == '^') return matchhere(re+1, text);
  do { if (matchhere(re, text)) return 1; } while (*text++ != '\0');
  return 0;
}
static int matchhere(char *re, char *text) {
  if (re[0] == '\0') return 1;
  if (re[1] == '*') return matchstar(re[0], re+2, text);
  if (re[0] == '$' && re[1] == '\0') return *text == '\0';
  if (*text != '\0' && (re[0] == '.' || re[0] == *text))
    return matchhere(re+1, text+1);
  return 0;
}
static int matchstar(int c, char *re, char *text) {
  do { if (matchhere(re, text)) return 1; }
  while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}
/* wrap so plain names match exactly (b -> ^b$); keep user anchors if present */
static void fullpat(char *in, char *out, int sz) {
  int n = strlen(in), i = 0;
  if (n == 0) { out[0]='^'; out[1]='$'; out[2]=0; return; }
  if (in[0] != '^' && i < sz-1) out[i++]='^';
  for (int j=0; j<n && i<sz-1; j++) out[i++]=in[j];
  if (in[n-1] != '$' && i < sz-1) out[i++]='$';
  out[i]=0;
}

/* --------- -exec support --------- */
static int exec_enabled = 0, basec = 0;
static char *basev[MAXARG];

static void run_exec(const char *fp) {
  if (!exec_enabled) return;
  char *argv[MAXARG]; int k=0;
  for (int i=0;i<basec && k<MAXARG-1;i++) argv[k++]=basev[i];
  if (k>=MAXARG-1){ fprintf(2,"find: exec argv too long\n"); return; }
  argv[k++]=(char*)fp; argv[k]=0;

  int pid=fork();
  if (pid==0){ exec(argv[0],argv); fprintf(2,"find: exec %s failed\n",argv[0]); exit(1); }
  else if (pid>0){ wait(0); }
  else { fprintf(2,"find: fork failed\n"); }
}

/* --------- recursive walk --------- */
static void rfind(const char *path, char *pat_full) {
  struct stat st;
  if (stat(path,&st)<0) return;

  if (st.type==T_FILE){
    const char *b=path+strlen(path);
    while (b>path && *(b-1)!='/') b--;
    if (match(pat_full,(char*)b)){ if (exec_enabled) run_exec(path); else printf("%s\n",path); }
    return;
  }
  if (st.type!=T_DIR) return;

  int fd=open(path,O_RDONLY); if (fd<0) return;

  char buf[512]; strcpy(buf,path);
  char *p=buf+strlen(buf); if (p==buf || *(p-1)!='/') *p++='/';

  struct dirent de;
  while (read(fd,&de,sizeof(de))==sizeof(de)){
    if (de.inum==0) continue;
    char name[DIRSIZ+1]; memmove(name,de.name,DIRSIZ); name[DIRSIZ]=0;
    if (!strcmp(name,".") || !strcmp(name,"..")) continue;

    char *q=p; int nlen=strlen(name); memmove(q,name,nlen); q[nlen]=0;

    if (stat(buf,&st)>=0){
      if (st.type==T_DIR) rfind(buf,pat_full);
      else if (st.type==T_FILE){
        if (match(pat_full,name)){ if (exec_enabled) run_exec(buf); else printf("%s\n",buf); }
      }
    }
    *p=0;
  }
  close(fd);
}

int main(int argc,char *argv[]){
  if (argc<3){
    fprintf(2,"usage: find <start-path> <pattern> [-exec <cmd> [args...]]\n");
    exit(1);
  }
  exec_enabled=0; basec=0;
  for (int i=3;i<argc;i++){
    if (!strcmp(argv[i],"-exec")){
      exec_enabled=1;
      for (int j=i+1;j<argc && basec<MAXARG-1;j++) basev[basec++]=argv[j];
      if (basec==0){ fprintf(2,"find: -exec needs a command\n"); exit(1); }
      break;
    }
  }
  char pat[128]; fullpat(argv[2],pat,sizeof pat);
  rfind(argv[1],pat);
  exit(0);
}
