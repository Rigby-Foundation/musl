/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Copyright (C) 2026 Rigby Foundation */
/* Runtime helpers the compiler expects from libgcc/compiler-rt on 32-bit
 * targets (64-bit division and shifts, 64-bit <-> floating point, the
 * instruction cache flush after writing code). Nothing ships them for a bare
 * powerpc-linux-musl clang target, so libc carries them; static links pick
 * up only what they reference. Empty on 64-bit targets. */
#if __SIZEOF_LONG__ == 4

typedef unsigned long long u64;
typedef long long s64;

static u64 udiv64(u64 n, u64 d, u64 *rem)
{
	u64 q = 0, r = 0;
	if (d == 0) { if (rem) *rem = 0; return 0; }
	for (int i = 63; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) { r -= d; q |= 1ULL << i; }
	}
	if (rem) *rem = r;
	return q;
}

u64 __udivdi3(u64 n, u64 d) { return udiv64(n, d, 0); }
u64 __umoddi3(u64 n, u64 d) { u64 r; udiv64(n, d, &r); return r; }
u64 __udivmoddi4(u64 n, u64 d, u64 *rem) { return udiv64(n, d, rem); }

s64 __divdi3(s64 n, s64 d)
{
	int neg = (n < 0) != (d < 0);
	u64 q = udiv64(n < 0 ? -(u64)n : (u64)n, d < 0 ? -(u64)d : (u64)d, 0);
	return neg ? -(s64)q : (s64)q;
}

s64 __moddi3(s64 n, s64 d)
{
	u64 r;
	udiv64(n < 0 ? -(u64)n : (u64)n, d < 0 ? -(u64)d : (u64)d, &r);
	return n < 0 ? -(s64)r : (s64)r;
}

u64 __ashldi3(u64 a, int b) { return b >= 32 ? (a << 32) << (b - 32) : b ? (a << b) : a; }
u64 __lshrdi3(u64 a, int b) { return b >= 32 ? (a >> 32) >> (b - 32) : b ? (a >> b) : a; }
s64 __ashrdi3(s64 a, int b) { return b >= 32 ? (a >> 32) >> (b - 32) : b ? (a >> b) : a; }

/* 64-bit integer <-> floating point: the FPU handles 32-bit conversions and
 * the doubles themselves; only the 64-bit widths need help. */
double __floatundidf(u64 a)
{
	return (double)(unsigned)(a >> 32) * 4294967296.0 + (double)(unsigned)a;
}
double __floatdidf(s64 a) { return a < 0 ? -__floatundidf(-(u64)a) : __floatundidf((u64)a); }
float  __floatundisf(u64 a) { return (float)__floatundidf(a); }
float  __floatdisf(s64 a) { return (float)__floatdidf(a); }

u64 __fixunsdfdi(double a)
{
	if (!(a > 0)) return 0;
	if (a >= 18446744073709551616.0) return ~0ULL;
	double hi = a / 4294967296.0;
	unsigned h = (unsigned)hi;
	return ((u64)h << 32) | (unsigned)(a - (double)h * 4294967296.0);
}
s64 __fixdfdi(double a) { return a < 0 ? -(s64)__fixunsdfdi(-a) : (s64)__fixunsdfdi(a); }
u64 __fixunssfdi(float a) { return __fixunsdfdi(a); }
s64 __fixsfdi(float a) { return __fixdfdi(a); }

#ifdef __powerpc__
/* __builtin___clear_cache: make freshly written instructions visible. */
void __clear_cache(void *start, void *end)
{
	unsigned long a = (unsigned long)start & ~31UL, e = (unsigned long)end;
	for (; a < e; a += 32) __asm__ __volatile__("dcbst 0,%0" :: "r"(a) : "memory");
	__asm__ __volatile__("sync");
	for (a = (unsigned long)start & ~31UL; a < e; a += 32) __asm__ __volatile__("icbi 0,%0" :: "r"(a) : "memory");
	__asm__ __volatile__("sync; isync");
}
#endif

#endif
