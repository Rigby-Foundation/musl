# libc — musl for sic

[musl](https://musl.libc.org) 1.2.5, ported to the sic system call ABI. This is
its own project, like a libc on Linux: the kernel ([sic](https://github.com/Rigby-Foundation/sic)) knows nothing
about it, and the userland ([ZAE](https://github.com/Rigby-Foundation/zae)) just links against it. The repo is a
full musl tree with the sic changes applied; `NOTICE` lists exactly which files.

## How the port works

sic has its own syscall table — `sic/abi/syscall.tbl` — which is deliberately
*not* Linux's numbering (sic is not a Linux clone; Linux binaries will not run).
The names are the POSIX/musl vocabulary, so musl needs almost no code changes:

- `arch/<arch>/bits/syscall.h.in` — generated from the table (`make abi`)
- the few `.s` files with hard-coded numbers (`clone`, `vfork`,
  `__set_thread_area`, `__unmapself`, `restore`) and, on x86_64, the vDSO
  hooks in `syscall_arch.h` removed
- x86_64 is built with `-fPIE`, because sic user programs live at
  `0x8000000000`; powerpc is plain non-PIC code at `0x10000000`
- `src/sic/builtins.c`: the 64-bit division/shift/float-conversion helpers a
  32-bit target expects from libgcc, since nothing ships them for a bare
  `powerpc-linux-musl` clang

Everything else — struct layouts, calling convention (`syscall` / `sc`,
`-errno` or CR0.SO+errno returns), TLS, the initial process stack with auxv —
is what musl's own port of that architecture expects and what the kernel
implements (`sic/include/abi/abi.h`, `sic/kernel/proc/syscall.c`). On 32-bit
targets the kernel is time64-only, so `make abi` for them also defines the
`*_time64` names musl looks for, as aliases of the plain numbers.

Ports: `x86_64` (default) and `powerpc` (32-bit big-endian, `make
ARCH=powerpc`, built with `-mcpu=7450 -maltivec`).

## Building

```bash
make            # GNUmakefile: cross-builds a static libc into build/ with clang
make install    # -> $SYSROOT/usr/include, $SYSROOT/usr/lib/{libc.a,crt1.o,crti.o,crtn.o}
```

All four sic projects meet in a **sysroot** rather than knowing each other's
paths: `$SIC_SYSROOT`, default `~/.sic/sysroot` (or `make SYSROOT=...`).
Install the kernel first (`make install` in
[sic](https://github.com/Rigby-Foundation/sic)) — it provides the syscall table
that `make abi` regenerates `arch/<arch>/bits/syscall.h.in` from. musl's own
`Makefile` and `configure` are untouched. Objects go to `obj/<arch>/`, the
result to `build/<arch>/`.

Compile programs with `clang --target=x86_64-linux-musl -nostdinc -isystem
$SYSROOT/usr/include -fPIE` and link with `ld.lld -static
--image-base=0x8000000000 crt1.o crti.o <objs> libc.a crtn.o` (see the ZAE
Makefile); for powerpc `--target=powerpc-linux-musl -mcpu=7450 -maltivec`
and `ld.lld -m elf32ppc --image-base=0x10000000`.

## What's missing on the kernel side

`mmap` is anonymous or private file-backed only (no shared mappings); no job
control (`setpgid` is a no-op); no `SIGSTOP`/`SIGCONT`. Sockets are `AF_INET`
only (TCP, UDP, raw ICMP; no `AF_UNIX`, no `socketpair`). Unimplemented
syscalls return `ENOSYS`.

## Contributing

Patches need a `Signed-off-by:` line (DCO 1.1). Read
[CODE_OF_CONFLICT](./CODE_OF_CONFLICT) and [CONTRIBUTING](./CONTRIBUTING).

## License

Upstream musl is Copyright (c) 2005-2020 Rich Felker et al., MIT (`COPYRIGHT`).
The sic changes are Copyright (C) 2026 Rigby Foundation under the LGPL-2.1-or-later
(`LICENSE`, `NOTICE`): a libc must not impose terms on the programs that link it.
