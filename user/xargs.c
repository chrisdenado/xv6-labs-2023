#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "argc %d", argc);
    exit(1);
  }

  char* cmd = argv[1];
  char* cmd_argv[MAXARG];
  memset(cmd_argv, 0, sizeof(cmd_argv));
  int cmd_argc = 0;
  for (int i=1; i<argc; ++i) {
    if (cmd_argc >= MAXARG - 1) {
      fprintf(2, "too many arguments\n");
      exit(1);
    }
    cmd_argv[cmd_argc++] = argv[i];
  }

  char buf[256];
  char* p = buf;
  int n = 0, len = sizeof(buf);
  while ((n = read(0, p, len)) > 0) {
    p += n;
    len -= n;    
  }
  if (len == 0) {
    printf("buf length(256) is not enough, please enlarge");
    exit(1);
  }
  *p = 0;

  p = buf;
  for (char* iter=buf; (*iter) != 0; ++iter) {
    if ((*iter) == '\n') {
      int len = iter - p + 1;
      char* data = malloc(len);
      memcpy(data, p, len);
      data[len-1] = 0;
      cmd_argv[cmd_argc++] = data;
      p = iter+1;
    }
  }
  cmd_argv[cmd_argc] = 0;

  if (fork() == 0) {
    exec(cmd, cmd_argv);
  } else {
    wait(0);
  }

  exit(0);
}
