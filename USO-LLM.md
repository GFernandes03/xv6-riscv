# Declaração de uso de LLM

Uso de LLM's para o trabalho por mim, Guilherme Fernandes, conforme abaixo:

## 1. Ferramentas

| Ferramenta | Modelo / versão | Plano | Uso |
|---|---|---|---|
| Gemini (Google) | [PREENCHER: versão mostrada no seletor de modelos do app] | gratuito | planejamento e orientação |
| Claude (Anthropic) | [PREENCHER: Claude Sonnet 5.5, conforme o seletor do app] | gratuito | código, testes, depuração e documentação |

## 2. Onde foi utilizada

**Gemini (sem geração de código):**
- Roteiro geral do trabalho a partir do enunciado.
- Como obter o hash completo do commit-base.
- Sugestão de combinação de comandos, com complexidade de cada um. As sugestões foram avaliadas e **a escolha final de uname, free, ps e pwd foi minha**.
- Mapa dos arquivos a alterar (`syscall.h`, `syscall.c`, `sysproc.c`/`sysfile.c`, `user.h`, `usys.pl`, `UPROGS`).

**Claude (código e depuração):**
- Cabeçalhos `kernel/uname.h`, `kernel/meminfo.h`, `kernel/pinfo.h`.
- Syscalls `sys_uname`, `sys_meminfo`, `sys_getprocs`, `sys_getcwd` e funções de apoio `freemem`, `totalmem` (`kalloc.c`), `getprocs` (`proc.c`).
- Programas `user/uname.c`, `user/free.c`, `user/ps.c`, `user/pwd.c` e o teste `user/testtrabalho.c`.
- Depuração a partir das saídas do `make` e do QEMU que colei na conversa.
- Rascunho dos documentos de entrega, revisado por mim.

## 3. Como foi utilizada

Enviei o PDF do enunciado, informei os quatro comandos e colei as mensagens de erro e as saídas dos testes. Não alterei o código gerado por conta própria além de aplicar as correções indicadas abaixo.

**Aceito:** estrutura geral das quatro syscalls, a ordem de locks no `getprocs` (`wait_lock` antes de `p->lock`, `copyout` fora da região crítica), o algoritmo do `getcwd` (subir por `..`), o plano de testes.

**Adaptado / rejeitado:**
- O Gemini sugeriu `sys_free`; usei `meminfo`, porque `free` colide com `free()` de `user/umalloc.c`.
- O Gemini apresentou `fa2ddcb` como o "commit original da MIT"; é o último commit do xv6 que eu tinha antes de começar, e o hash completo foi obtido com `git log -1 --format=%H fa2ddcb`.

## 4. Verificação e erros encontrados

O código do Claude foi escrito **sem ser compilado**; os erros apareceram ao compilar e testar:

| # | Erro | Como foi detectado | Correção |
|---|---|---|---|
| 1 | `printf` com largura de campo (`%8ld`), que o `printf` do xv6 não suporta | revisão do próprio Claude antes da compilação | `%d` com conversão para `int` |
| 2 | Sequência de teste do `pwd` com `;` e sem considerar a falta de `PATH` | execução no QEMU (`exec mkdir failed`) | uma linha por vez e caminho absoluto (`/pwd`) |
| 3 | `copyout` com 4 argumentos; o repositório tem 5 (`psz`, alocação preguiçosa) | erro de compilação (`-Werror`) | `p->sz` inserido nas 4 chamadas |
| 4 | `sleep()` usado no teste, não declarado no `user.h` do repositório | erro de compilação | trocado pela chamada equivalente declarada (`pause`) |
| 5 | Explicação inicial do Claude para o `exec ... failed` ("ruído de colar texto") estava errada; o usuário havia digitado | questionamento do usuário e testes A e B | explicação retirada; causa registrada como **não determinada** (ver README, seção 8) |

Validação final: `testtrabalho` com 39 PASS e 0 FAIL, `usertests` com ALL TESTS PASSED, `free` estável em 20 ciclos, `ps` sem zumbis, `make clean && make qemu` sem erros.

Li as funções entregues e consigo explicar o caminho `ecall` → `usertrap` → `syscall()` → `sys_*` → retorno, o papel de `copyout`/`psz`, a ordem de locks do `getprocs` e as referências de inode do `getcwd`. [CONFIRMAR antes de assinar]
