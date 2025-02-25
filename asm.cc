#include <sys/mman.h>
#include <string.h>

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

static void patch(Assembler &a, u32 addr, u64 v, u8 count)
{
	for (u32 i = 0; i < count; i++)
		a.code[addr+i] = (v & (0xfflu<<(i*8)))>>(i*8);
}

void push_bytes(Assembler &a, u64 v, u8 count)
{
	if (!a.code)
		a.code = (u8 *)mmap(0, CodeSize, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);
	if (a.ip + count < a.ip) {
		a.err = AsmErrOverflow;
		return;
	}
	patch(a, a.ip, v, count);
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
	return s;
}

static Symbol *add_sym(Assembler &a, const char *name)
{
	Symbol *s = (Symbol *)alloc(a.tmp, sizeof(Symbol));
	*s = {a.syms, 0, name, 0, 0};
	a.syms = s;
	return s;
}

static void add_ref(Assembler &a, Symbol *s)
{
	Ref *r = (Ref *)alloc(a.tmp, sizeof(Ref));
	*r = {s->refs, a.ip};
	s->refs = r;
}

void label(Assembler &a, const char *name)
{
	Symbol *s = get_sym(a, name);
	if (!s)
		s = add_sym(a, name);
	if (s->resolved)
		a.err = AsmErrDupLabel;
	s->resolved = 1;
	s->addr = a.ip;
	for (Ref *r = s->refs; r; r = r->next)
		patch(a, r->addr, s->addr - (r->addr+4), 4);
	s->refs = 0;
}

void push_label_offset(Assembler &a, const char *name)
{
	Symbol *s = get_sym(a, name);
	if (!s)
		s = add_sym(a, name);
	if (s->resolved) {
		push_bytes(a, s->addr - (a.ip+4), 4);
	} else {
		add_ref(a, s);
		push_bytes(a, 0, 4);
	}
}
