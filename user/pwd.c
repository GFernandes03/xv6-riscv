#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  char buf[256];

  if (getcwd(buf, sizeof(buf)) < 0) {
    fprintf(2, "pwd: erro ao obter diretório\n");
    exit(1);
  }
  printf("%s\n", buf);
  exit(0);
}