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

BUILD  := $(CURDIR)/build
OBJDIR := $(CURDIR)/obj
TARGET := x86_64-linux-musl
SYSROOT ?= $(if $(SIC_SYSROOT),$(SIC_SYSROOT),$(HOME)/.sic/sysroot)
ABI    := $(SYSROOT)/usr/share/sic/abi

.PHONY: all abi install clean distclean

all: $(BUILD)/lib/libc.a

install: all
	@mkdir -p $(SYSROOT)/usr/include $(SYSROOT)/usr/lib
	cp -R $(BUILD)/include/. $(SYSROOT)/usr/include/
	cp $(BUILD)/lib/libc.a $(BUILD)/lib/crt1.o $(BUILD)/lib/crti.o $(BUILD)/lib/crtn.o $(SYSROOT)/usr/lib/
	@echo "installed into $(SYSROOT)"

$(OBJDIR)/config.mak: configure
	@mkdir -p $(OBJDIR)
	cd $(OBJDIR) && CC="$(CC) --target=$(TARGET)" AR="$(AR)" RANLIB="$(RANLIB)" \
	    CFLAGS="-fPIE -O2 -g" \
	    ../configure --target=$(TARGET) --prefix=$(BUILD) \
	        --disable-shared --disable-gcc-wrapper --srcdir=..

$(BUILD)/lib/libc.a: $(OBJDIR)/config.mak $(shell find src arch include crt -type f)
	$(MAKE) -C $(OBJDIR) -j8 AR="$(AR)" RANLIB="$(RANLIB)" install

abi:
	python3 $(ABI)/gen.py musl > arch/x86_64/bits/syscall.h.in

clean:
	rm -rf $(OBJDIR) $(BUILD)

distclean: clean
