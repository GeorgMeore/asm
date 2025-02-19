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

static void addref(Symbol *s, u32 addr)
{
	Ref *r = (Ref *)malloc(sizeof(*r));
	r->next = s->refs;
	r->addr = addr;
	s->refs = r;
}

void clear(Assembler &a)
{
	while (a.symtab) {
		Symbol *s = a.symtab;
		a.symtab = s->next;
		free(s);
	}
	free(a.b);
	a = {};
}

static void patchbytes(Assembler &a, u32 addr, u64 v, u8 bytes)
{
	for (u32 i = 0; i < bytes; i++)
		a.b[addr+i] = (v & (0xfflu<<(i*8)))>>(i*8);
}

void pushbytes(Assembler &a, u64 v, u8 bytes)
{
	if (a.ip + bytes > a.cap) {
		a.cap += 4096; // why not
		a.b = (u8 *)realloc(a.b, a.cap);
	}
	patchbytes(a, a.ip, v, bytes);
	a.ip += bytes;
}

void pushbyte(Assembler &a, u8 b)
{
	pushbytes(a, b, 1);
}

static Symbol *getsym(Assembler &a, const char *name)
{
	Symbol *s = a.symtab;
	while (s && strcmp(name, s->name))
		s = s->next;
	return s;
}

static Symbol *addsym(Assembler &a, const char *name)
{
	Symbol *s = sym(name);
	s->next = a.symtab;
	a.symtab = s;
	return s;
}

void label(Assembler &a, const char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	if (s->resolved)
		panic("label already defined");
	s->resolved = 1;
	s->addr = a.ip;
	while (s->refs) {
		Ref *r = s->refs;
		s->refs = r->next;
		patchbytes(a, r->addr, s->addr - (r->addr+4), 4);
		free(r);
	}
}

void pushlabeloffset(Assembler &a, const char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	if (s->resolved) {
		pushbytes(a, s->addr - (a.ip+4), 4);
	} else {
		addref(s, a.ip);
		pushbytes(a, 0, 4);
	}
}
