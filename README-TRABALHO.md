# Trabalho de SO - Extensão do xv6-RISC-V implementando: uname, free, ps e pwd

## 1. Identificação

- **Integrante:** Guilherme Fernandes (trabalho individual) — matrícula: 20230001015
- **Disciplina:** Sistemas Operacionais — UFRRJ
- **Repositório base:** https://github.com/mit-pdos/xv6-riscv
- **Commit-base:** `fa2ddcb7ca87a77af579380a6eafd5aff163b44f` ("usertests: bound the zero-length pipe read regression test")
- Depois do commit-base há dois commits só de documentação (`4578653`, `189a907`). As modificações de código começam em `c197d8c`, e a correção da assinatura de `copyout` está em `ad758e2`.

Verificação de que os comandos não existiam na base (deve imprimir nada):

```bash
git ls-tree -r --name-only fa2ddcb user | grep -E '/(uname|free|ps|pwd)\.c$'
git show fa2ddcb:kernel/syscall.h | grep -E 'uname|meminfo|getprocs|getcwd'
```

Resultado: os dois comandos não imprimem nenhuma linha no commit-base fa2ddcb,
confirmando que uname, free, ps e pwd (e as syscalls correspondentes) não existiam.

## 2. Ambiente e como compilar

| Item | Versão |
|---|---|
| Sistema | Windows com WSL2, Ubuntu 26.04.1 LTS |
| Compilador | riscv64-linux-gnu-gcc 15.2.0 (Ubuntu 15.2.0-16ubuntu1) |
| Emulador | QEMU 10.2.1 (Debian 1:10.2.1+ds-1ubuntu3.3), pacote `qemu-system-riscv` |

Pacotes: `build-essential git qemu-system-riscv gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu perl`.

```bash
make clean
make qemu TOOLPREFIX=riscv64-linux-gnu-
```

Para sair do QEMU: `Ctrl+A` e depois `X`.

## 3. Arquivos

**Novos:** `kernel/uname.h`, `kernel/meminfo.h`, `kernel/pinfo.h`, `user/uname.c`, `user/free.c`, `user/ps.c`, `user/pwd.c`, `user/testtrabalho.c`.

**Modificados:** `Makefile` (UPROGS), `kernel/syscall.h`, `kernel/syscall.c`, `kernel/defs.h`, `kernel/sysproc.c`, `kernel/sysfile.c`, `kernel/kalloc.c`, `kernel/proc.c`, `user/user.h`, `user/usys.pl`.

Conferir a lista: `git diff --stat fa2ddcb`.

## 4. Interfaces

Todas as chamadas retornam -1 em erro. Ponteiros de usuário são acessados só por `copyout`; o retorno é sempre verificado.

| Comando | Syscall (nº) | Assinatura | Retorno |
|---|---|---|---|
| uname | `uname` (22) | `int uname(struct utsname*)` | 0, ou -1 se o endereço for inválido |
| free | `meminfo` (23) | `int meminfo(struct meminfo*)` | 0, ou -1 se o endereço for inválido |
| ps | `getprocs` (24) | `int getprocs(struct pinfo*, int max)` | nº de entradas copiadas (>= 0), ou -1 |
| pwd | `getcwd` (25) | `int getcwd(char *buf, int size)` | 0, ou -1 |

Observação: `meminfo` foi usado no lugar de `free` para evitar colisão com `free()` de `user/umalloc.c`.

### uname
`struct utsname` tem 5 campos `char[32]`: sysname, nodename, release, version, machine. A estrutura é zerada com `memset` antes do preenchimento para não vazar a pilha do kernel. Valores: `xv6`, `xv6-host`, `1.0`, `UFRRJ-SO-2026`, `riscv64`.

### meminfo (free)
`struct meminfo { uint64 total, free; }`, em **bytes**. `free` é o número de páginas na lista livre do `kalloc` vezes `PGSIZE`, contado com `kmem.lock` retido. `total` é `PGROUNDDOWN(PHYSTOP) - PGROUNDUP(end)`. O programa converte para KB. "Usada" é `total - free` e inclui tudo o que o kernel alocou com `kalloc` (tabelas de páginas, pilhas do kernel, buffers de pipe), não só memória de usuário.

### getprocs (ps)
`struct pinfo { int pid, ppid, state; char name[16]; uint64 sz; }`. `state` é o valor numérico de `enum procstate` (UNUSED=0 … ZOMBIE=5); `sz` está em bytes. Entradas UNUSED são puladas. Retorna no máximo `min(max, NPROC)`. Erros: `max <= 0` ou falha de `copyout` (as entradas já copiadas permanecem no buffer do usuário). Sincronização: `wait_lock` antes de `p->lock`, como no resto do `proc.c`, e o `copyout` só acontece depois de soltar os dois locks. O resultado é um retrato **não atômico** da tabela de processos.

### getcwd (pwd)
Sobe de `..` em `..` a partir do `cwd` do processo, procurando em cada pai a entrada de mesmo inode, até chegar à raiz (`ROOTINO`). Usa `begin_op`/`end_op` porque `iput` pode escrever em disco. `size` inclui o `'\0'`; `size <= 0`, caminho que não cabe em `size` ou em `MAXPATH`, ou `cwd` já removido retornam -1.

## 5. Exemplos de uso (saídas obtidas)

```
$ uname -a
xv6 xv6-host 1.0 UFRRJ-SO-2026 riscv64
$ free
        total      usada      livre   (KB)
Mem: 130924 800 130124
$ ps
PID  PPID  ESTADO  MEM(KB)  NOME
1 0 sleep 16 init
2 1 sleep 20 sh
45 2 run 16 ps
$ cd a
$ /pwd
/a
```

O xv6 não tem `PATH`: o `sh` procura o programa no diretório atual. Dentro de um subdiretório, chame `/pwd`, `/mkdir` etc. com caminho absoluto.

## 6. Simplificações em relação ao Linux

- `uname`: 5 campos fixos de 32 bytes, valores constantes; só a opção `-a`.
- `free`: só memória física do alocador; sem swap, buffers/cache nem opções.
- `ps`: sem `/proc`; syscall didática `getprocs`; sem opções; retrato não atômico.
- `pwd`: sem links simbólicos, sem `-L`/`-P`; sem suporte a `cwd` removido (retorna -1).
- Nenhuma compatibilidade binária com as syscalls do Linux.

## 7. Testes

Dentro do QEMU, a partir da raiz:

```
$ testtrabalho
$ usertests
```

- `testtrabalho`: **39 PASS, 0 FAIL**. Cobre sucesso, ponteiro não mapeado (`0x3000000000`) e ponteiro em `MAXVA` (`0x4000000000`) sem derrubar o kernel, `max <= 0`, buffers curtos/zero/negativos, `cwd` removido, efeito real (`sbrk` de 256 KB reduz o livre em exatamente 256 KB, `sbrk(-N)` devolve; `fork`+`wait` não vaza páginas; filho criado aparece no `getprocs` com `ppid` correto e some após `wait`) e concorrência (4 processos x 50 iterações das três syscalls).
- `usertests`: **ALL TESTS PASSED** (sem regressão).
- Manual: 20 ciclos de `echo`/`pwd`/`ps`/`free`; `free` ficou em `130924/800/130124` e o `ps` nunca mostrou processos sobrando.

## 8. Limitações conhecidas

1. **`exec <programa> failed` esporádico, causa não determinada.** Observado nos primeiros comandos após o boot: no Teste A (40 execuções de `echo 1`, sem usar nenhuma syscall nova) houve 1 falha; em outras sessões houve falhas com `pwd`/`echo`, em duas delas com um byte estranho no nome impresso (`exec ��pwd failed`). Como o Teste A não usou as syscalls novas, elas não explicam a falha, mas **não foi feita a comparação com o xv6 original (commit-base)**, então não se afirma que o problema vem da base. A memória física estável e a ausência de processos zumbis são as evidências de que as syscalls não vazam recursos.
2. Testes executados com a configuração padrão do Makefile (3 CPUs); não foram repetidos com `CPUS=1`.
3. Ausência de `PATH` no `sh` do xv6 (ver seção 5).
