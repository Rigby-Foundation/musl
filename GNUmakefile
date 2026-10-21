# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (C) 2026 Rigby Foundation
#
# sic build wrapper for this musl tree. GNU make prefers GNUmakefile over
# Makefile, so `make` here cross-builds a static libc for sic with clang;
# musl's own Makefile is untouched for the usual `./configure && make` flow.
#
# `make` builds into build/ (headers + static libs); `make install` copies
# them into the sysroot shared with the kernel and userland:
#   $(SYSROOT)/usr/include, $(SYSROOT)/usr/lib/{libc.a,crt1.o,crti.o,crtn.o}
#   (plus the empty libm.a, libpthread.a, ... so third-party builds' -lm link)
# Programs link with ld.lld -static --image-base=0x8000000000.
#
# `make abi` regenerates arch/x86_64/bits/syscall.h.in from the kernel's
# syscall table, which `make install` in the sic tree puts into the sysroot.

LLVM_PREFIX ?= $(shell brew --prefix llvm 2>/dev/null)
ifeq ($(origin CC),default)
CC := $(LLVM_PREFIX)/bin/clang
endif
ifeq ($(origin AR),default)
AR := $(LLVM_PREFIX)/bin/llvm-ar
endif
RANLIB ?= $(LLVM_PREFIX)/bin/llvm-ranlib
LLD_PREFIX ?= $(firstword $(foreach f,lld lld@21 lld@20 llvm,$(if $(wildcard $(shell brew --prefix $(f) 2>/dev/null)/bin/ld.lld),$(shell brew --prefix $(f)),)))
LD_LLD ?= $(if $(LLD_PREFIX),$(LLD_PREFIX)/bin/ld.lld,ld.lld)

# ARCH selects the musl port: x86_64 (default) or powerpc (32-bit big-endian).
ARCH   ?= x86_64
BUILD  := $(CURDIR)/build/$(ARCH)
OBJDIR := $(CURDIR)/obj/$(ARCH)
ifeq ($(ARCH),powerpc)
TARGET := powerpc-linux-musl
ARCH_CFLAGS := -mcpu=7450 -maltivec
ABI_FLAVOUR := musl32
else ifeq ($(ARCH),aarch64)
TARGET := aarch64-linux-musl
ARCH_CFLAGS := -fPIE
ABI_FLAVOUR := musl
else
TARGET := x86_64-linux-musl
ARCH_CFLAGS := -fPIE
ABI_FLAVOUR := musl
endif
# The compiler's runtime builtins, which clang does not ship for these
# targets: aarch64's binary128 long double (printf needs __multf3, ...), and
# 128-bit integer division everywhere (libc++'s filesystem clock). Zig has
# them: its compiler-rt, bundled into an archive of its own.
COMPILER_RT := $(BUILD)/lib/libcompiler_rt.a
SYSROOT ?= $(if $(SIC_SYSROOT),$(SIC_SYSROOT),$(HOME)/.sic/sysroot)
ABI    := $(SYSROOT)/usr/share/sic/abi

.PHONY: all abi install clean distclean

# The shared libc, which is also the dynamic linker (musl's libc.so is its
# own ld.so, installed as /lib/ld-musl-$(ARCH).so.1). 64-bit ports only for now.
SHARED := $(if $(filter x86_64 aarch64,$(ARCH)),$(BUILD)/lib/libc.so,)
SHOBJDIR := $(OBJDIR)-shared

all: $(BUILD)/lib/libc.a $(COMPILER_RT) $(SHARED)

$(BUILD)/lib/libcompiler_rt.a: | $(BUILD)/lib/libc.a
	@mkdir -p $(OBJDIR)/rt
	echo 'pub export fn sic_compiler_rt_anchor() void {}' > $(OBJDIR)/rt/rt.zig
	cd $(OBJDIR)/rt && zig build-lib rt.zig -target $(TARGET) -OReleaseFast -fPIC -fcompiler-rt --name compiler_rt   # PIC: sic loads programs above 2 GiB
	cp $(OBJDIR)/rt/libcompiler_rt.a $@

install: all
	@mkdir -p $(SYSROOT)/usr/include $(SYSROOT)/usr/lib
	cp -R $(BUILD)/include/. $(SYSROOT)/usr/include/
	cp $(BUILD)/lib/libc.a $(BUILD)/lib/crt1.o $(BUILD)/lib/crti.o $(BUILD)/lib/crtn.o $(SYSROOT)/usr/lib/
	@[ -z "$(COMPILER_RT)" ] || cp $(COMPILER_RT) $(SYSROOT)/usr/lib/
	@if [ -n "$(SHARED)" ]; then \
	    cp $(BUILD)/lib/libc.so $(BUILD)/lib/Scrt1.o $(SYSROOT)/usr/lib/ && \
	    mkdir -p $(SYSROOT)/lib && ln -sf ../usr/lib/libc.so $(SYSROOT)/lib/ld-musl-$(ARCH).so.1 && \
	    echo "installed libc.so (and /lib/ld-musl-$(ARCH).so.1)"; fi
	cp $(BUILD)/lib/libm.a $(BUILD)/lib/libpthread.a $(BUILD)/lib/librt.a $(BUILD)/lib/libdl.a $(BUILD)/lib/libutil.a $(BUILD)/lib/libcrypt.a $(SYSROOT)/usr/lib/   # empty: everything is in libc.a, for -lm and friends
	@echo "installed into $(SYSROOT)"

$(OBJDIR)/config.mak: configure
	@mkdir -p $(OBJDIR)
	cd $(OBJDIR) && CC="$(CC) --target=$(TARGET)" AR="$(AR)" RANLIB="$(RANLIB)" \
	    CFLAGS="$(ARCH_CFLAGS) -O2 -g" \
	    $(CURDIR)/configure --target=$(TARGET) --prefix=$(BUILD) \
	        --disable-shared --disable-gcc-wrapper --srcdir=$(CURDIR)

$(BUILD)/lib/libc.a: $(OBJDIR)/config.mak $(shell find src arch include crt -type f)
	@mkdir -p $(BUILD)
	$(MAKE) -C $(OBJDIR) -j8 AR="$(AR)" RANLIB="$(RANLIB)" install

# musl's link steps (and configure's linker probes, which ignore LDFLAGS) run
# the compiler driver's default `ld`; on a Mac that is Apple's. Put lld first
# under that name instead.
LDSHIM := $(OBJDIR)-ldshim
SHPATH := $(LDSHIM):$(PATH)
$(LDSHIM)/ld:
	@mkdir -p $(LDSHIM)
	ln -sf $(LD_LLD) $@

$(SHOBJDIR)/config.mak: configure | $(LDSHIM)/ld
	@mkdir -p $(SHOBJDIR)
	cd $(SHOBJDIR) && PATH="$(SHPATH)" CC="$(CC) --target=$(TARGET)" AR="$(AR)" RANLIB="$(RANLIB)" \
	    CFLAGS="-O2 -g" \
	    $(CURDIR)/configure --target=$(TARGET) --prefix=$(BUILD)/shared \
	        --enable-shared --disable-static --disable-gcc-wrapper --srcdir=$(CURDIR)

# sic executables (and so their interpreter) carry OS/ABI 'S' (0x53);
# lld takes it from the objects, which this compiler target does not set.
$(BUILD)/lib/libc.so: $(SHOBJDIR)/config.mak $(COMPILER_RT) $(shell find src arch include crt ldso -type f)
	PATH="$(SHPATH)" $(MAKE) -C $(SHOBJDIR) -j8 AR="$(AR)" RANLIB="$(RANLIB)" LIBCC="$(COMPILER_RT)" lib/libc.so lib/Scrt1.o
	@mkdir -p $(BUILD)/lib
	cp $(SHOBJDIR)/lib/libc.so $(SHOBJDIR)/lib/Scrt1.o $(BUILD)/lib/
	printf 'S' | dd of=$@ bs=1 seek=7 count=1 conv=notrunc status=none

abi:
	python3 $(ABI)/gen.py $(ABI_FLAVOUR) > arch/$(ARCH)/bits/syscall.h.in

clean:
	rm -rf $(OBJDIR) $(BUILD)

distclean: clean
