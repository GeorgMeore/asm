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

uchar modrm(uchar mod, uchar reg, uchar rm)
{
	return mod<<6 | reg<<3 | rm;
}

uchar sib(uchar scale, uchar index, uchar base)
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

typedef enum {
	JMP  = 0, /* unconditional */
	JA   = 0x87,
	JAE  = 0x83,
	JB   = 0x82,
	JBE  = 0x86,
	JC   = 0x82,
	JE   = 0x84,
	JG   = 0x8F,
	JGE  = 0x8D,
	JL   = 0x8C,
	JLE  = 0x8E,
	JNA  = 0x86,
	JNAE = 0x82,
	JNB  = 0x83,
	JNBE = 0x87,
	JNC  = 0x83,
	JNE  = 0x85,
	JNG  = 0x8E,
	JNGE = 0x8C,
	JNL  = 0x8D,
	JNLE = 0x8F,
	JNO  = 0x81,
	JNP  = 0x8B,
	JNS  = 0x89,
	JNZ  = 0x85,
	JO   = 0x80,
	JP   = 0x8A,
	JPE  = 0x8A,
	JPO  = 0x8B,
	JS   = 0x88,
} Jcond;

void jmpcond(Assembler *a, Operand op, Jcond c)
{
	assert(op.type == OpLabel, "inderect jumps are not supported yet");
	if (c == JMP) {
		pushbyte(a, 0xE9);
	} else {
		pushbyte(a, 0x0F);
		pushbyte(a, c);
	}
	Symbol *s = getsym(a, op.p);
	if (!s)
		s = addsym(a, op.p);
	if (s->resolved) {
		pushbytes(a, s->addr - (a->ip+4), 4);
	} else {
		addref(s, a->ip);
		pushbytes(a, 0, 4);
	}
}

void je(Assembler *a, Operand op)
{
	jmpcond(a, op, JE);
}

void jmp(Assembler *a, Operand op)
{
	jmpcond(a, op, JMP);
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
	pushbyte(a, sib(0b00, 0b100, indir.reg));
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

typedef enum {
	ArithAdd = 0b000,
	ArithSub = 0b101,
	ArithCmp = 0b111,
} Arith;

void arith(Assembler *a, Operand src, Operand dst, Arith op)
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
	arith(a, src, dst, ArithAdd);
}

void sub(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, ArithSub);
}

void cmp(Assembler *a, Operand src, Operand dst)
{
	arith(a, src, dst, ArithCmp);
}

void ret(Assembler *a)
{
	pushbyte(a, 0xc3);
}

void *code(Assembler *a)
{
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

int main(void)
{
	Assembler *a = asmb();
	fib(a);
	//for (uint i = 0; i < a->ip; i++)
	//	putchar(a->b[i]);
	long (*f)(long) = code(a);
	printf("%ld\n", f(1));
	asmfree(a);
	return 0;
}
