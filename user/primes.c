#include "kernel/types.h"
#include "user/user.h"

// void prime_func() __attribute__((noreturn));

void prime_func() {
  int prime = -1;
  int n = read(0, &prime, 4);
  if (n < 0 || prime == -1) {
    fprintf(2, "read error\n");
    exit(1);
  }
  fprintf(1, "prime %d\n", prime);

  int p[2], has_fork=0;
  while (1) {
    int recv_n = -1;
    n = read(0, &recv_n, 4);
    if (n < 0) {
      fprintf(2, "read error[2]\n");
      exit(1);
    }
    if (recv_n == 0) {
      write(1, &recv_n, 4); // bypass ending flag to subprocess
      // as we arrive the end, we can close 0&1 to save fd resources.
      // otherwise we may meet read error for large number.
      close(0); close(1);
      return;
    }
    if (recv_n % prime == 0) {
      continue;
    }

    if (has_fork == 0) {
      pipe(p);
      int pid = fork();
      if (pid == 0) {
        close(p[1]);
        close(0); dup(p[0]); close(p[0]);
        char *primes_argv[] = { 0 };
        exec("primes", primes_argv);
      } else {
        close(p[0]);
        close(1); dup(p[1]); close(p[1]);
        has_fork = 1;
      }
    }
    write(1, &recv_n, 4);
  }
}

int main(int argc, char *argv[])
{  
  if (argc == 0) { // primes sub-process
    prime_func();
    wait(0); // wait sub-processes stop
    exit(0);
  }

  int num = 280; // default is 280
  if (argc == 2) {
    num = atoi(argv[1]);
  }
  int p[2];
  pipe(p);
  int pid = fork();
  if (pid == 0) {
    close(p[1]);
    close(0); dup(p[0]); close(p[0]); // dup p[0] -> 0
    char *primes_argv[] = { 0 };
    exec("primes", primes_argv);
  } else {
    close(p[0]);
    close(1); dup(p[1]); close(p[1]); // dup p[1] -> 1
    for (int i=2; i<=num; ++i) {
      write(1, &i, 4);
    }
    // use 0 as ending flag, to notice subprocess to stop.
    int i=0;
    write(1, &i, 4);
    wait(0);
  }
  exit(0);
}