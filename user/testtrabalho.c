#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/uname.h"
#include "kernel/meminfo.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define BADPTR  ((void*)0x3000000000UL)  // abaixo de MAXVA, acima de p->sz: nunca mapeado
#define HUGEPTR ((void*)0x4000000000UL)  // == MAXVA: fora do espaço de endereçamento
#define MAXP 64

static int pass, fail;
static struct pinfo plist[MAXP];

static void
check(int cond, char *msg)
{
  if(cond){
    printf("  PASS: %s\n", msg);
    pass++;
  } else {
    printf("  FAIL: %s\n", msg);
    fail++;
  }
}

static void
touch(char *p, int n)
{
  for(int i = 0; i < n; i += 4096)
    p[i] = 1;
}

static void
test_uname(void)
{
  struct utsname u;
  printf("uname:\n");
  check(uname(&u) == 0, "chamada valida retorna 0");
  check(strcmp(u.sysname, "xv6") == 0, "sysname == xv6");
  check(strcmp(u.machine, "riscv64") == 0, "machine == riscv64");
  check(uname((struct utsname*)BADPTR) == -1, "ponteiro nao mapeado retorna -1");
  check(uname((struct utsname*)HUGEPTR) == -1, "ponteiro >= MAXVA retorna -1");
}

static void
test_meminfo(void)
{
  struct meminfo a, b, c;
  int N = 64 * 4096;
  char *p;

  printf("meminfo/free:\n");
  check(meminfo(&a) == 0, "chamada valida retorna 0");
  check(a.total > 0 && a.free > 0 && a.free <= a.total, "0 < free <= total");
  check(meminfo((struct meminfo*)BADPTR) == -1, "ponteiro nao mapeado retorna -1");
  check(meminfo((struct meminfo*)HUGEPTR) == -1, "ponteiro >= MAXVA retorna -1");

  p = sbrk(N);
  if(p == (char*)-1){
    check(0, "sbrk(N) falhou");
    return;
  }
  touch(p, N);
  meminfo(&b);
  printf("  livre antes=%d KB, depois do sbrk=%d KB\n",
         (int)(a.free / 1024), (int)(b.free / 1024));
  check(b.free + N <= a.free, "free diminui pelo menos N apos sbrk+uso");
  sbrk(-N);
  meminfo(&c);
  check(c.free >= b.free + N, "free volta a subir apos sbrk(-N)");

  // fork + exit + wait nao pode vazar paginas
  meminfo(&a);
  int pid = fork();
  if(pid == 0)
    exit(0);
  wait(0);
  meminfo(&b);
  printf("  livre antes do fork=%d KB, depois do wait=%d KB\n",
         (int)(a.free / 1024), (int)(b.free / 1024));
  check(a.free == b.free, "fork+wait nao vaza paginas");
}

static int
find(int n, int pid)
{
  for(int i = 0; i < n; i++)
    if(plist[i].pid == pid)
      return i;
  return -1;
}

static void
test_getprocs(void)
{
  int n, i, pid;

  printf("getprocs/ps:\n");
  n = getprocs(plist, MAXP);
  check(n >= 3, "lista ao menos init, sh e este processo");
  i = find(n, getpid());
  check(i >= 0, "o proprio pid aparece");
  if(i >= 0){
    check(plist[i].state == 4, "o proprio estado e RUNNING (4)");
    check(strcmp(plist[i].name, "testtrabalho") == 0, "nome correto");
    check(plist[i].sz > 0, "tamanho > 0");
  }
  check(getprocs(plist, 0) == -1, "max == 0 retorna -1");
  check(getprocs(plist, -5) == -1, "max < 0 retorna -1");
  check(getprocs(plist, 1) == 1, "max == 1 retorna 1");
  check(getprocs((struct pinfo*)BADPTR, MAXP) == -1, "ponteiro nao mapeado retorna -1");
  check(getprocs((struct pinfo*)HUGEPTR, MAXP) == -1, "ponteiro >= MAXVA retorna -1");

  // processo criado e encerrado
  pid = fork();
  if(pid == 0){
    pause(200);
    exit(0);
  }
  n = getprocs(plist, MAXP);
  i = find(n, pid);
  check(i >= 0, "filho aparece enquanto vive");
  if(i >= 0)
    check(plist[i].ppid == getpid(), "ppid do filho == pid do pai");
  kill(pid);
  wait(0);
  n = getprocs(plist, MAXP);
  check(find(n, pid) < 0, "filho some depois do wait");
}

static void
test_getcwd(void)
{
  char buf[128];
  int pid, st;

  printf("getcwd/pwd:\n");
  unlink("/tt_a/b");
  unlink("/tt_a");
  chdir("/");
  check(getcwd(buf, sizeof(buf)) == 0 && strcmp(buf, "/") == 0, "raiz retorna /");
  check(mkdir("/tt_a") == 0 && mkdir("/tt_a/b") == 0, "cria /tt_a/b");
  check(chdir("/tt_a/b") == 0, "chdir /tt_a/b");
  check(getcwd(buf, sizeof(buf)) == 0 && strcmp(buf, "/tt_a/b") == 0, "retorna /tt_a/b");
  check(getcwd(buf, 8) == 0, "buffer exato (7 + NUL) funciona");
  check(getcwd(buf, 7) == -1, "buffer 1 byte curto retorna -1");
  check(getcwd(buf, 2) == -1, "buffer pequeno retorna -1");
  check(getcwd(buf, 0) == -1, "tamanho 0 retorna -1");
  check(getcwd(buf, -1) == -1, "tamanho negativo retorna -1");
  check(getcwd((char*)BADPTR, 128) == -1, "ponteiro nao mapeado retorna -1");
  check(getcwd((char*)HUGEPTR, 128) == -1, "ponteiro >= MAXVA retorna -1");
  chdir("/tt_a");
  check(getcwd(buf, sizeof(buf)) == 0 && strcmp(buf, "/tt_a") == 0, "retorna /tt_a");

  // cwd removido: o filho remove o proprio diretorio
  pid = fork();
  if(pid == 0){
    chdir("/tt_a/b");
    unlink("/tt_a/b");
    exit(getcwd(buf, sizeof(buf)) == -1 ? 0 : 1);
  }
  wait(&st);
  check(st == 0, "cwd removido retorna -1");

  chdir("/");
  unlink("/tt_a/b");
  unlink("/tt_a");
}

static void
test_concurrency(void)
{
  int nproc = 4, st, ok = 1;
  struct meminfo m;
  char buf[64];

  printf("concorrencia (%d processos):\n", nproc);
  for(int k = 0; k < nproc; k++){
    if(fork() == 0){
      for(int j = 0; j < 50; j++){
        if(getprocs(plist, MAXP) < 1) exit(1);
        if(meminfo(&m) < 0 || m.free > m.total) exit(1);
        if(getcwd(buf, sizeof(buf)) < 0) exit(1);
      }
      exit(0);
    }
  }
  for(int k = 0; k < nproc; k++){
    wait(&st);
    if(st != 0)
      ok = 0;
  }
  check(ok, "chamadas simultaneas retornam resultados validos");
}

int
main(void)
{
  test_uname();
  test_meminfo();
  test_getprocs();
  test_getcwd();
  test_concurrency();
  printf("\nResultado: %d PASS, %d FAIL\n", pass, fail);
  exit(fail ? 1 : 0);
}