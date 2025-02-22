#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.hh"
#include "asm.hh"

static void panic(const char *msg)
{
	fprintf(stderr, "Assembler error: %s\n", msg);
	exit(1);
}

static Symbol *sym(const char *name)
{
	Symbol *s = (Symbol *)malloc(sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->name = name;
	return s;
}

static void add_ref(Symbol *s, u32 addr)
{
	Ref *r = (Ref *)malloc(sizeof(*r));
	r->next = s->refs;
	r->addr = addr;
	s->refs = r;
}

void clear(Assembler &a)
{
	while (a.syms) {
		Symbol *s = a.syms;
		a.syms = s->next;
		free(s);
	}
	free(a.b);
	a = {};
}

static void patch(Assembler &a, u32 addr, u64 v, u8 count)
{
	for (u32 i = 0; i < count; i++)
		a.b[addr+i] = (v & (0xfflu<<(i*8)))>>(i*8);
}

void push(Assembler &a, u64 v, u8 count)
{
	if (a.ip + count > a.cap) {
		a.cap += 4096; // why not
		a.b = (u8 *)realloc(a.b, a.cap);
	}
	patch(a, a.ip, v, count);
	a.ip += count;
}

void push(Assembler &a, u8 b)
{
	push(a, b, 1);
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
	Symbol *s = sym(name);
	s->next = a.syms;
	a.syms = s;
	return s;
}

void label(Assembler &a, const char *name)
{
	Symbol *s = get_sym(a, name);
	if (!s)
		s = add_sym(a, name);
	if (s->resolved)
		panic("label already defined");
	s->resolved = 1;
	s->addr = a.ip;
	while (s->refs) {
		Ref *r = s->refs;
		s->refs = r->next;
		patch(a, r->addr, s->addr - (r->addr+4), 4);
		free(r);
	}
}

void push_label_offset(Assembler &a, const char *name)
{
	Symbol *s = get_sym(a, name);
	if (!s)
		s = add_sym(a, name);
	if (s->resolved) {
		push(a, s->addr - (a.ip+4), 4);
	} else {
		add_ref(s, a.ip);
		push(a, 0, 4);
	}
}
