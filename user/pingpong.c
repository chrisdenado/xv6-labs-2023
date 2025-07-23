#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int p[2];
  char c[256];
  pipe(p);
  if (fork() == 0) {
    int n = read(p[0], &c, 1);
    int pid = getpid();
    if (n < 0) {
      printf("[%d] process read failed!\n", pid);
      exit(1);
    }
    close(p[0]);
    if (n == 1) {
      fprintf(1, "%d: received ping\n", pid);
      write(p[1], "a", 1);
    }
    close(p[1]);
  } else {
    int pid = getpid();
    write(p[1], "a", 1);
    close(p[1]);
    wait(0);
    int n = read(p[0], &c, 1);
    if (n < 0) {
      printf("[%d] process read failed!\n", pid);
      exit(1);
    }
    close(p[0]);
    if (n == 1) {
      fprintf(1, "%d: received pong\n", pid);
    }
  }

  exit(0);
}