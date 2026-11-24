struct meminfo {
  uint64 total;   // bytes de memória física gerenciada pelo alocador
  uint64 free;    // bytes livres (páginas na freelist * PGSIZE)
};