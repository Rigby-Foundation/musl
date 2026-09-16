# libc — musl for sic

[musl](https://musl.libc.org) 1.2.5, ported to the sic system call ABI. This is
its own project, like a libc on Linux: the kernel ([sic](https://github.com/Rigby-Foundation/sic)) knows nothing
about it, and the userland ([ZAE](https://github.com/Rigby-Foundation/zae)) just links against it. The repo is a
full musl tree with the sic changes applied; `NOTICE` lists exactly which files.

## How the port works

sic has its own syscall table — `sic/abi/syscall.tbl` — which is deliberately
*not* Linux's numbering (sic is not a Linux clone; Linux binaries will not run).
The names are the POSIX/musl vocabulary, so musl needs almost no code changes:

- `arch/x86_64/bits/syscall.h.in` — generated from the table (`make abi`)
- five x86_64 `.s` files with hard-coded numbers (`clone`, `vfork`,
  `__set_thread_area`, `__unmapself`, `restore`) and the vDSO hooks in
  `syscall_arch.h` removed
- built with `-fPIE`, because sic user programs live at `0x8000000000`

Everything else — struct layouts, calling convention (`syscall` instruction,
`-errno` returns), TLS via `arch_prctl(ARCH_SET_FS)`, the initial process stack
with auxv — is what musl's x86_64 port expects and what the kernel implements
(`sic/include/abi/abi.h`, `sic/kernel/proc/syscall.c`).

## Building

```bash
make            # GNUmakefile: cross-builds a static libc into build/ with clang
make install    # -> $SYSROOT/usr/include, $SYSROOT/usr/lib/{libc.a,crt1.o,crti.o,crtn.o}
```

All four sic projects meet in a **sysroot** rather than knowing each other's
paths: `$SIC_SYSROOT`, default `~/.sic/sysroot` (or `make SYSROOT=...`).
Install the kernel first (`make install` in
[sic](https://github.com/Rigby-Foundation/sic)) — it provides the syscall table
that `make abi` regenerates `arch/x86_64/bits/syscall.h.in` from. musl's own
`Makefile` and `configure` are untouched.

Compile programs with `clang --target=x86_64-linux-musl -nostdinc -isystem
$SYSROOT/usr/include -fPIE` and link with `ld.lld -static
--image-base=0x8000000000 crt1.o crti.o <objs> libc.a crtn.o` (see the ZAE
Makefile).

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
