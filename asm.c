#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/user.h>
#include <sys/mman.h>

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

void panic(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

void assert(int v, char *msg)
{
	if (!v)
		panic(msg);
}

typedef struct Ref Ref;
struct Ref {
	Ref  *next;
	uint addr;
};

typedef struct Symbol Symbol;
struct Symbol {
	Symbol *next;
	uint   addr;
	char   *name;
	Ref    *refs;
	int    resolved;
};

Symbol *sym(char *name)
{
	Symbol *s = malloc(sizeof(*s));
	memset(s, 0, sizeof(*s));
	s->name = name;
	return s;
}

void symfree(Symbol *s)
{
	free(s);
}

void addref(Symbol *s, uint addr)
{
	Ref *r = malloc(sizeof(*r));
	r->next = s->refs;
	r->addr = addr;
	s->refs = r;
}

typedef struct {
	Symbol *symtab;
	char   *b;
	uint   cap;
	uint   ip;
} Assembler;

Assembler *asmb(void)
{
	Assembler *a = malloc(sizeof(*a));
	memset(a, 0, sizeof(*a));
	a->cap = PAGE_SIZE;
	a->b = malloc(a->cap);
	return a;
}

void asmfree(Assembler *a)
{
	while (a->symtab) {
		Symbol *s = a->symtab;
		a->symtab = s->next;
		symfree(s);
	}
	free(a->b);
	free(a);
}

void patchbytes(Assembler *a, uint addr, ulong v, uint bytes)
{
	for (uint i = 0; i < bytes; i++)
		a->b[addr+i] = (v & (0xFFLU<<(i*8)))>>(i*8);
}

void pushbytes(Assembler *a, ulong v, uint bytes)
{
	if (a->ip + bytes > a->cap) {
		a->cap += PAGE_SIZE;
		a->b = realloc(a->b, a->cap);
	}
	patchbytes(a, a->ip, v, bytes);
	a->ip += bytes;
}

void pushbyte(Assembler *a, uchar b)
{
	pushbytes(a, b, 1);
}

Symbol *getsym(Assembler *a, char *name)
{
	Symbol *s = a->symtab;
	while (s && strcmp(name, s->name))
		s = s->next;
	return s;
}

Symbol *addsym(Assembler *a, char *name)
{
	Symbol *s = sym(name);
	s->next = a->symtab;
	a->symtab = s;
	return s;
}

typedef enum {
	REXW = 0b01001000,
	REXR = 0b01000100,
	REXX = 0b01000010,
	REXB = 0b01000001,
} REX;

typedef enum {
	ModDisp0 = 0b00,
	ModDisp1 = 0b01,
	ModDisp4 = 0b10,
	ModDirect = 0b11,
} Mod;

uchar modrm(Mod mod, uchar reg, uchar rm)
{
	return mod<<6 | reg<<3 | rm;
}

typedef enum {
	Scale1 = 0b00,
	Scale2 = 0b01,
	Scale4 = 0b10,
	Scale8 = 0b11,
} Scale;

uchar sib(Scale scale, uchar index, uchar base)
{
	return scale<<6 | index<<3 | base;
}

typedef enum {
	OpReg,
	OpMoffs,
	OpImm,
	OpIndir,
	OpLabel,
} Optype;

typedef struct {
	Optype type;
	uchar reg;
	ulong v;
	void  *p;
} Operand;

Operand rax = {.type = OpReg, .reg = 0b000};
Operand rcx = {.type = OpReg, .reg = 0b001};
Operand rdx = {.type = OpReg, .reg = 0b010};
Operand rbx = {.type = OpReg, .reg = 0b011};
Operand rsp = {.type = OpReg, .reg = 0b100};
Operand rbp = {.type = OpReg, .reg = 0b101};
Operand rsi = {.type = OpReg, .reg = 0b110};
Operand rdi = {.type = OpReg, .reg = 0b111};

Operand imm(ulong v)
{
	return (Operand){.type = OpImm, .v = v};
}

Operand addr(void *p)
{
	return (Operand){.type = OpMoffs, .p = p};
}

Operand label(char *name)
{
	return (Operand){.type = OpLabel, .p = name};
}

Operand indir(Operand reg, uint offset)
{
	Operand op = reg;
	op.type = OpIndir;
	op.v = offset;
	return op;
}

Operand deref(Operand reg)
{
	return indir(reg, 0);
}

void setlabel(Assembler *a, char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	assert(!s->resolved, "label already defined");
	s->resolved = 1;
	s->addr = a->ip;
	while (s->refs) {
		Ref *r = s->refs;
		s->refs = r->next;
		patchbytes(a, r->addr, s->addr - (r->addr+4), 4);
		free(r);
	}
}

void pushlabeloffset(Assembler *a, char *name)
{
	Symbol *s = getsym(a, name);
	if (!s)
		s = addsym(a, name);
	if (s->resolved) {
		pushbytes(a, s->addr - (a->ip+4), 4);
	} else {
		addref(s, a->ip);
		pushbytes(a, 0, 4);
	}
}

void jump(Assembler *a, Operand op, uchar oci, uchar ocl)
{
	if (op.type == OpReg) {
		pushbyte(a, 0xFF);
		pushbyte(a, modrm(ModDirect, oci, op.reg));
	} else if (op.type == OpIndir) {
		pushbyte(a, 0xFF);
		pushbyte(a, modrm(ModDisp4, oci, 0b100));
		pushbyte(a, sib(Scale1, 0b100, op.reg));
		pushbytes(a, op.v, 4);
	} else if (op.type == OpLabel) {
		pushbyte(a, ocl);
		pushlabeloffset(a, op.p);
	} else
		panic("unsupported operand");
}

void jmp(Assembler *a, Operand op)
{
	jump(a, op, 0b100, 0xE9);
}

void jcc(Assembler *a, Operand op, uchar c)
{
	assert(op.type == OpLabel, "conditional jumps only work with labels");
	pushbyte(a, 0x0F);
	pushbyte(a, c);
	pushlabeloffset(a, op.p);
}

void je(Assembler *a, Operand op)
{
	jcc(a, op, 0x84);
}

void call(Assembler *a, Operand op)
{
	jump(a, op, 0b010, 0xE8);
}

typedef enum {
	DirRRM = 0b001,
	DirRMR = 0b011,
} Direction;

void movregmoffs(Assembler *a, Operand reg, Operand moffs, Direction d)
{
	assert(reg.reg == rax.reg, "can only move from/to address via A");
	pushbyte(a, 0xA0 + d);
	pushbytes(a, (ulong)moffs.p, 8);
}

void movregindir(Assembler *a, Operand reg, Operand indir, Direction d)
{
	pushbyte(a, 0x88 + d);
	pushbyte(a, modrm(ModDisp4, reg.reg, 0b100));
	pushbyte(a, sib(Scale1, 0b100, indir.reg));
	pushbytes(a, indir.v, 4);
}

void mov(Assembler *a, Operand src, Operand dst)
{
	pushbyte(a, REXW);
	if (src.type == OpReg && dst.type == OpMoffs)
		movregmoffs(a, src, dst, DirRMR);
	else if (dst.type == OpReg && src.type == OpMoffs)
		movregmoffs(a, dst, src, DirRRM);
	else if (src.type == OpImm && dst.type == OpReg) {
		pushbyte(a, 0xB8 + dst.reg);
		pushbytes(a, src.v, 8);
	} else if (src.type == OpReg && dst.type == OpIndir)
		movregindir(a, src, dst, DirRRM);
	else if (dst.type == OpReg && src.type == OpIndir)
		movregindir(a, dst, src, DirRMR);
	else if (src.type == OpReg && dst.type == OpReg) {
		pushbyte(a, 0x88 + DirRRM);
		pushbyte(a, modrm(ModDirect, src.reg, dst.reg));
	} else
		panic("unsupported operand types");
}

void arith(Assembler *a, Operand src, Operand dst, uchar op)
{
	pushbyte(a, REXW);
	if (src.type == OpReg && dst.type == OpReg) {
		pushbyte(a, (op<<3) + DirRRM);
		pushbyte(a, modrm(ModDirect, src.reg, dst.reg));
	} else if (src.type == OpImm && dst.type == OpReg) {
		pushbyte(a, 0x81);
		pushbyte(a, modrm(ModDirect, op, dst.reg));
		pushbytes(a, src.v, 4);
	} else
		panic("unsupported operand types");
}

void add(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b000);
}

void or(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b001);
}

void and(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b100);
}

void sub(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b101);
}

void xor(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b110);
}

void cmp(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, 0b111);
}

void lea(Assembler *a, Operand src, Operand dst)
{
	assert(dst.type == OpReg, "can only lea into registers");
	assert(src.type == OpIndir, "only indirect register expressions supported");
	pushbyte(a, REXW);
	pushbyte(a, 0x8D);
	pushbyte(a, modrm(ModDisp4, dst.reg, 0b100));
	pushbyte(a, sib(Scale1, 0b100, src.reg));
	pushbytes(a, src.v, 4);
}

void push(Assembler *a, Operand op)
{
	assert(op.type == OpReg, "can only push from registers");
	pushbyte(a, 0x50 + op.reg);
}

void pop(Assembler *a, Operand op)
{
	assert(op.type == OpReg, "can only pop to registers");
	pushbyte(a, 0x58 + op.reg);
}

void ret(Assembler *a)
{
	pushbyte(a, 0xc3);
}

void *code(Assembler *a)
{
	for (Symbol *s = a->symtab; s; s = s->next) {
		if (s->refs)
			panic("found unresolved references");
	}
	void *p = mmap(0, a->ip, PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	memcpy(p, a->b, a->ip);
	mprotect(p, a->ip, PROT_READ|PROT_EXEC);
	return p;
}

void fib(Assembler *a)
{
	mov(a, imm(0), rax);
	mov(a, imm(1), rbx);
setlabel(a, "loop");
	cmp(a, imm(0), rdi);
	je(a, label("return"));
	mov(a, rbx, rdx);
	add(a, rax, rbx);
	mov(a, rdx, rax);
	sub(a, imm(1), rdi);
	jmp(a, label("loop"));
setlabel(a, "return");
	ret(a);
}

void sum(Assembler *a)
{
setlabel(a, "sum");
	cmp(a, imm(0), rdi);
	je(a, label("return0"));
	push(a, rdi);
	sub(a, imm(1), rdi);
	call(a, label("sum"));
	pop(a, rdi);
	add(a, rdi, rax);
	ret(a);
setlabel(a, "return0");
	xor(a, rax, rax);
	ret(a);
}

int main(void)
{
	Assembler *a = asmb();
	//fib(a);
	sum(a);
	//for (uint i = 0; i < a->ip; i++)
	//	putchar(a->b[i]);
	long (*f)(long) = code(a);
	printf("%ld\n", f(6));
	asmfree(a);
	return 0;
}
