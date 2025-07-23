#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "sleep function need a interger argument\n");
    exit(1);
  }
  
  int sec = atoi(argv[1]);
  sleep(sec); // Ticks counter increments with 0.1sec

  exit(0);
}