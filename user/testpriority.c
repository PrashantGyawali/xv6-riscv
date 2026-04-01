#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid1, pid2;
  
  printf("Starting priority scheduler test...\n");

  // Set the parent process priority to 10
  setpriority(getpid(), 10);

  pid1 = fork();
  if(pid1 == 0){
    // Child 1 gets priority 20 (lower priority)
    setpriority(getpid(), 20); 
    for(volatile int i = 0; i < 100000000; i++) {
      if(i % 25000000 == 0)
        printf("Process 1 (Priority 20) running...\n");
    }
    printf("Process 1 finished\n");
    exit(0);
  }

  pid2 = fork();
  if(pid2 == 0){
    // Child 2 gets priority 5 (higher priority)
    setpriority(getpid(), 5); 
    for(volatile int i = 0; i < 100000000; i++) {
      if(i % 25000000 == 0)
        printf("Process 2 (Priority 5) running...\n");
    }
    printf("Process 2 finished\n");
    exit(0);
  }

  wait(0);
  wait(0);
  printf("Priority scheduler test finished.\n");
  exit(0);
}
