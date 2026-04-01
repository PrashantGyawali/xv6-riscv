#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  char buf[128];
  if (getcwd(buf, sizeof(buf)) == 0) {
    printf("getcwd failed\n");
    exit(1);
  }
  printf("cwd: %s\n", buf);
  exit(0);
}