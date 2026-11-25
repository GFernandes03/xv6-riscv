#include "kernel/types.h"
#include "kernel/meminfo.h"
#include "user/user.h"

int
main(void)
{
  struct meminfo m;

  if (meminfo(&m) < 0) {
    fprintf(2, "free: falha na syscall\n");
    exit(1);
  }
  printf("        total      usada      livre   (KB)\n");
  printf("Mem: %d %d %d\n", (int)(m.total / 1024),
         (int)((m.total - m.free) / 1024), (int)(m.free / 1024));
  exit(0);
}