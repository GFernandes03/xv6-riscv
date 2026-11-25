#include "kernel/types.h"
#include "kernel/uname.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct utsname u;

  if (uname(&u) < 0) {
    fprintf(2, "uname: falha na syscall\n");
    exit(1);
  }
  if (argc > 1 && strcmp(argv[1], "-a") == 0)
    printf("%s %s %s %s %s\n", u.sysname, u.nodename, u.release, u.version,
           u.machine);
  else
    printf("%s\n", u.sysname);
  exit(0);
}