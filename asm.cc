#include <sys/mman.h>
#include <string.h>
#include <assert.h>

#include "types.hh"
#include "arena.hh"
#include "asm.hh"

// We just reserve 4GiB of space upfront
static const u64 CodeSize = 4*((u64)1 << 30);

void clear(Assembler &a)
{
	reset(a.tmp);
	munmap(a.code, CodeSize);
	a = {};
}

void push_bytes(Assembler &a, u64 v, u8 count)
{
	if (!a.code)
		a.code = (u8 *)mmap(0, CodeSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);
	assert(a.code != MAP_FAILED);
	if (a.ip > CodeSize - count) {
		a.err = ErrOverflow;
		return;
	}
	for (u8 i = 0; i < count; i++) {
		a.code[a.ip+i] = v & 0xff;
		v = v >> 8;
	}
	a.ip += count;
}

void push_byte(Assembler &a, u8 b)
{
	push_bytes(a, b, 1);
}

static Symbol *get_sym(Assembler &a, const char *name)
{
	Symbol *s = a.syms;
	while (s && strcmp(name, s->name))
		s = s->next;
	if (!s) {
		s = (Symbol *)alloc(a.tmp, sizeof(Symbol));
		*s = {a.syms, 0, name, 0, 0};
		a.syms = s;
	}
	return s;
}

static void patch_ref(Assembler &a, u32 addr, u32 pos, u32 sub, u32 div, u8 len, u8 off)
{
	u32 n = (off + len + 7)/8;
	if (pos > a.ip || a.ip - pos < n) {
		a.err = ErrOverflow;
		return;
	}
	s64 v = (s64)addr - (s64)sub;
	if (!len || len > 32 || off >= 32 || !div || v % div) {
		a.err = ErrPatchParam;
		return;
	}
	v /= div;
	u64 mask = (u64)-1 >> (64 - len);
	// check that all dropped bits equal to the sign bit
	if ((v & (~mask))>>1 != (v & ((~mask)>>1))) {
		a.err = ErrOverflow;
		return;
	}
	v = (v & mask) << off;
	for (u8 i = 0; i < n; i++) {
		a.code[pos+i] |= v & 0xff;
		v = v >> 8;
	}
}

void label(Assembler &a, const char *name)
{
	Symbol *s = get_sym(a, name);
	if (s->resolved) {
		a.err = ErrDupLabel;
		return;
	}
	s->resolved = 1;
	s->addr = a.ip;
	for (Ref *r = s->refs; r; r = r->next)
		patch_ref(a, s->addr, r->pos, r->sub, r->div, r->len, r->off);
	s->refs = 0;
}

void label_ref(Assembler &a, const char *name, u32 pos, u32 sub, u32 div, u8 len, u8 off)
{
	Symbol *s = get_sym(a, name);
	if (s->resolved) {
		patch_ref(a, s->addr, pos, sub, div, len, off);
	} else {
		Ref *r = (Ref *)alloc(a.tmp, sizeof(Ref));
		*r = {s->refs, pos, sub, div, len, off};
		s->refs = r;
	}
}
