#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "uname.h"
#include "meminfo.h"

uint64
sys_uname(void)
{
  uint64 addr;
  struct utsname u;

  argaddr(0, &addr);
  memset(&u, 0, sizeof(u));          // não vazar lixo da pilha do kernel
  safestrcpy(u.sysname,  "xv6", sizeof(u.sysname));
  safestrcpy(u.nodename, "xv6-host", sizeof(u.nodename));
  safestrcpy(u.release,  "1.0", sizeof(u.release));
  safestrcpy(u.version,  "UFRRJ-SO-2026", sizeof(u.version));
  safestrcpy(u.machine,  "riscv64", sizeof(u.machine));

  if(copyout(myproc()->pagetable, addr, (char*)&u, sizeof(u)) < 0)
    return -1;
  return 0;
}

uint64
sys_meminfo(void)
{
  uint64 addr;
  struct meminfo m;

  argaddr(0, &addr);
  m.total = totalmem();
  m.free  = freemem();
  if(copyout(myproc()->pagetable, addr, (char*)&m, sizeof(m)) < 0)
    return -1;
  return 0;
}

uint64
sys_getprocs(void)
{
  uint64 addr;
  int max;

  argaddr(0, &addr);
  argint(1, &max);
  if(max <= 0)
    return -1;
  return getprocs(addr, max);
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
