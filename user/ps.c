#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define MAXP 64

static struct pinfo list[MAXP];
static char *states[] = {"unused", "used", "sleep", "runble", "run", "zombie"};

int
main(void)
{
  int n = getprocs(list, MAXP);
  if (n < 0) {
    fprintf(2, "ps: falha na syscall\n");
    exit(1);
  }
  printf("PID  PPID  ESTADO  MEM(KB)  NOME\n");
  for (int i = 0; i < n; i++) {
    printf("%d %d %s %d %s\n", list[i].pid, list[i].ppid, states[list[i].state],
           (int)(list[i].sz / 1024), list[i].name);
  }
  exit(0);
}