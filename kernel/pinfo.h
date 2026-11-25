struct pinfo {
  int pid;
  int ppid;
  int state; // enum procstate
  char name[16];
  uint64 sz; // bytes de memória do processo
};